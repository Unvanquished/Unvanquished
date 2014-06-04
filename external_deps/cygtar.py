#!/usr/bin/env python2
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import hashlib
import optparse
import os
import posixpath
import shutil
import subprocess
import stat
import sys
import tarfile

"""A Cygwin aware version compress/extract object.

This module supports creating and unpacking a tarfile on all platforms.  For
Cygwin, Mac, and Linux, it will use the standard tarfile implementation.  For
Win32 it will detect Cygwin style symlinks as it archives and convert them to
symlinks.

For Win32, it is unfortunate that os.stat does not return a FileID in the ino
field which would allow us to correctly determine which files are hardlinks, so
instead we assume that any files in the archive that are an exact match are
hardlinks to the same data.

We know they are not Symlinks because we are using Cygwin style symlinks only,
which appear to Win32 a normal file.

All paths stored and retrieved from a TAR file are expected to be POSIX style,
Win32 style paths will be rejected.

NOTE:
  All paths represent by the tarfile and all API functions are POSIX style paths
  except for CygTar.Add which assumes a Native path.
"""


def ToNativePath(native_path):
  """Convert to a posix style path if this is win32."""
  if sys.platform == 'win32':
    return native_path.replace('/', '\\')
  return native_path


def IsCygwinSymlink(symtext):
  """Return true if the provided text looks like a Cygwin symlink."""
  return symtext[:12] == '!<symlink>\xff\xfe'


def SymDatToPath(symtext):
  """Convert a Cygwin style symlink data to a relative path."""
  return ''.join([ch for ch in symtext[12:] if ch != '\x00'])


def PathToSymDat(filepath):
  """Convert a filepath to cygwin style symlink data."""
  symtag = '!<symlink>\xff\xfe'
  unipath = ''.join([ch + '\x00' for ch in filepath])
  strterm = '\x00\x00'
  return symtag + unipath + strterm


def CreateCygwinSymlink(filepath, target):
  """Create a Cygwin 1.7 style link

  Generates a Cygwin style symlink by creating a SYSTEM tagged
  file with the !<link> marker followed by a unicode path.
  """
  # If we failed to create a symlink, then just copy it.  We wrap this in a
  # retry for Windows which often has stale file lock issues.
  for cnt in range(1,4):
    try:
      lnk = open(filepath, 'wb')
      lnk.write(PathToSymDat(target))
      lnk.close()
      break
    except EnvironmentError:
      print 'Try %d: Failed open %s -> %s\n' % (cnt, filepath, target)

  # Verify the file was created
  if not os.path.isfile(filepath):
    print 'Try %d: Failed create %s -> %s\n' % (cnt, filepath, target)
    print 'Giving up.'
    return False

  # Now set the system attribute bit so that Cygwin knows it's a link.
  for cnt in range(1,4):
    try:
      return subprocess.call(['cmd', '/C', 'C:\\Windows\\System32\\attrib.exe',
                              '+S', ToNativePath(filepath)])
    except EnvironmentError:
      print 'Try %d: Failed attrib %s -> %s\n' % (cnt, filepath, target)
  print 'Giving up.'
  return False


def CreateWin32Hardlink(filepath, targpath, try_mklink):
  """Create a hardlink on Win32 if possible

  Uses mklink to create a hardlink if possible.  On failure, it will
  assume mklink is unavailible and copy the file instead, returning False
  to indicate future calls should not attempt to use mklink."""

  # Assume an error, if subprocess succeeds, then it should return 0
  err = 1
  if try_mklink:
    dst_src = ToNativePath(filepath) + ' ' + ToNativePath(targpath)
    try:
      err = subprocess.call(['cmd', '/C', 'mklink /H ' + dst_src],
                            stdout = open(os.devnull, 'wb'))
    except EnvironmentError:
      try_mklink = False

  # If we failed to create a hardlink, then just copy it.  We wrap this in a
  # retry for Windows which often has stale file lock issues.
  if err or not os.path.isfile(filepath):
    for cnt in range(1,4):
      try:
        shutil.copyfile(targpath, filepath)
        return False
      except EnvironmentError:
        print 'Try %d: Failed hardlink %s -> %s\n' % (cnt, filepath, targpath)
    print 'Giving up.'
  return try_mklink


def ComputeFileHash(filepath):
  """Generate a sha1 hash for the file at the given path."""
  sha1 = hashlib.sha1()
  with open(filepath, 'rb') as fp:
    sha1.update(fp.read())
  return sha1.hexdigest()


def ReadableSizeOf(num):
  """Convert to a human readable number."""
  if num < 1024.0:
    return '[%5dB]' % num
  for x in ['B','K','M','G','T']:
     if num < 1024.0:
       return '[%5.1f%s]' % (num, x)
     num /= 1024.0
  return '[%dT]' % int(num)


class CygTar(object):
  """ CygTar is an object which represents a Win32 and Cygwin aware tarball."""
  def __init__(self, filename, mode='r', verbose=False):
    self.size_map = {}
    self.file_hashes = {}
    # Set errorlevel=1 so that fatal errors actually raise!
    self.tar = tarfile.open(filename, mode, errorlevel=1)
    self.verbose = verbose

  def __DumpInfo(self, tarinfo):
    """Prints information on a single object in the tarball."""
    typeinfo = '?'
    lnk = ''
    if tarinfo.issym():
      typeinfo = 'S'
      lnk = '-> ' + tarinfo.linkname
    if tarinfo.islnk():
      typeinfo = 'H'
      lnk = '-> ' + tarinfo.linkname
    if tarinfo.isdir():
      typeinfo = 'D'
    if tarinfo.isfile():
      typeinfo = 'F'
    reable_size = ReadableSizeOf(tarinfo.size)
    print '%s %s : %s %s' % (reable_size, typeinfo, tarinfo.name, lnk)
    return tarinfo

  def __AddFile(self, tarinfo, fileobj=None):
    """Add a file to the archive."""
    if self.verbose:
      self.__DumpInfo(tarinfo)
    self.tar.addfile(tarinfo, fileobj)

  def __AddLink(self, tarinfo, linktype, linkpath):
    """Add a Win32 symlink or hardlink to the archive."""
    tarinfo.linkname = linkpath
    tarinfo.type = linktype
    tarinfo.size = 0
    self.__AddFile(tarinfo)

  def Add(self, filepath, prefix=None):
    """Add path filepath to the archive which may be Native style.

    Add files individually recursing on directories.  For POSIX we use
    tarfile.addfile directly on symlinks and hardlinks.  For files, we
    must check if they are duplicates which we convert to hardlinks
    or Cygwin style symlinks which we convert form a file to a symlink
    in the tarfile.  All other files are added as a standard file.
    """

    # At this point tarinfo.name will contain a POSIX style path regardless
    # of the original filepath.
    tarinfo = self.tar.gettarinfo(filepath)
    if prefix:
      tarinfo.name = posixpath.join(prefix, tarinfo.name)

    if sys.platform == 'win32':
      # On win32 os.stat() always claims that files are world writable
      # which means that unless we remove this bit here we end up with
      # world writables files in the archive, which is almost certainly
      # not intended.
      tarinfo.mode &= ~stat.S_IWOTH
      tarinfo.mode &= ~stat.S_IWGRP

      # If we want cygwin to be able to extract this archive and use
      # executables and dll files we need to mark all the archive members as
      # executable.  This is essentially what happens anyway when the
      # archive is extracted on win32.
      tarinfo.mode |= stat.S_IXUSR | stat.S_IXOTH | stat.S_IXGRP

    # If this a symlink or hardlink, add it
    if tarinfo.issym() or tarinfo.islnk():
      tarinfo.size = 0
      self.__AddFile(tarinfo)
      return True

    # If it's a directory, then you want to recurse into it
    if tarinfo.isdir():
      self.__AddFile(tarinfo)
      native_files = glob.glob(os.path.join(filepath, '*'))
      for native_file in native_files:
        if not self.Add(native_file, prefix): return False
      return True

    # At this point we only allow addition of "FILES"
    if not tarinfo.isfile():
      print 'Failed to add non real file: %s' % filepath
      return False

    # Now check if it is a Cygwin style link disguised as a file.
    # We go ahead and check on all platforms just in case we are tar'ing a
    # mount shared with windows.
    if tarinfo.size <= 524:
      with open(filepath) as fp:
        symtext = fp.read()
      if IsCygwinSymlink(symtext):
        self.__AddLink(tarinfo, tarfile.SYMTYPE, SymDatToPath(symtext))
        return True

    # Otherwise, check if its a hardlink by seeing if it matches any unique
    # hash within the list of hashed files for that file size.
    nodelist = self.size_map.get(tarinfo.size, [])

    # If that size bucket is empty, add this file, no need to get the hash until
    # we get a bucket collision for the first time..
    if not nodelist:
      self.size_map[tarinfo.size] = [filepath]
      with open(filepath, 'rb') as fp:
        self.__AddFile(tarinfo, fp)
      return True

    # If the size collides with anything, we'll need to check hashes.  We assume
    # no hash collisions for SHA1 on a given bucket, since the number of files
    # in a bucket over possible SHA1 values is near zero.
    newhash = ComputeFileHash(filepath)
    self.file_hashes[filepath] = newhash

    for oldname in nodelist:
      oldhash = self.file_hashes.get(oldname, None)
      if not oldhash:
        oldhash = ComputeFileHash(oldname)
        self.file_hashes[oldname] = oldhash

      if oldhash == newhash:
        self.__AddLink(tarinfo, tarfile.LNKTYPE, oldname)
        return True

    # Otherwise, we missed, so add it to the bucket for this size
    self.size_map[tarinfo.size].append(filepath)
    with open(filepath, 'rb') as fp:
      self.__AddFile(tarinfo, fp)
    return True

  def Extract(self):
    """Extract the tarfile to the current directory."""
    try_mklink = True

    if self.verbose:
      sys.stdout.write('|' + ('-' * 48) + '|\n')
      sys.stdout.flush()
      div = float(len(self.tar.getmembers())) / 50.0
      dots = 0
      cnt = 0

    for m in self.tar:
      if self.verbose:
        cnt += 1
        while float(dots) * div < float(cnt):
          dots += 1
          sys.stdout.write('.')
          sys.stdout.flush()

      # For symlinks in Windows we create Cygwin 1.7 style symlinks since the
      # toolchain is Cygwin based.  For hardlinks on Windows, we use mklink if
      # possible to create a hardlink. For all other tar items, or platforms we
      # go ahead and extract it normally.
      if m.issym() and sys.platform == 'win32':
        CreateCygwinSymlink(m.name, m.linkname)
      # For hardlinks in Windows, we try to use mklink, and instead copy on
      # failure.
      elif m.islnk() and sys.platform == 'win32':
        try_mklink = CreateWin32Hardlink(m.name, m.linkname, try_mklink)
      # Otherwise, extract normally.
      else:
        if m.issym() or m.islnk():
          m.linkname = m.linkname.replace('\\', '/')
        self.tar.extract(m)
    if self.verbose:
      sys.stdout.write('\n')
      sys.stdout.flush()

  def List(self):
    """List the set of objects in the tarball."""
    for tarinfo in self.tar:
      self.__DumpInfo(tarinfo)

  def Close(self):
    self.tar.close()


def Main(args):
  parser = optparse.OptionParser()
  # Modes
  parser.add_option('-c', '--create', help='Create a tarball.',
      action='store_const', const='c', dest='action', default='')
  parser.add_option('-x', '--extract', help='Extract a tarball.',
      action='store_const', const='x', dest='action')
  parser.add_option('-t', '--list', help='List sources in tarball.',
      action='store_const', const='t', dest='action')

  # Compression formats
  parser.add_option('-j', '--bzip2', help='Create a bz2 tarball.',
      action='store_const', const=':bz2', dest='format', default='')
  parser.add_option('-z', '--gzip', help='Create a gzip tarball.',
      action='store_const', const=':gz', dest='format', )
  # Misc
  parser.add_option('-v', '--verbose', help='Use verbose output.',
      action='store_true', dest='verbose', default=False)
  parser.add_option('-f', '--file', help='Name of tarball.',
      dest='filename', default='')
  parser.add_option('-C', '--directory', help='Change directory.',
      dest='cd', default='')
  parser.add_option('--prefix', help='Subdirectory prefix for all paths')

  options, args = parser.parse_args(args[1:])
  if not options.action:
    parser.error('Expecting compress or extract')
  if not options.filename:
    parser.error('Expecting a filename')

  if options.action in ['c'] and not args:
    parser.error('Expecting list of sources to add')
  if options.action in ['x', 't'] and args:
    parser.error('Unexpected source list on extract')

  if options.action == 'c':
    mode = 'w' + options.format
  else:
    mode = 'r'+ options.format

  tar = CygTar(options.filename, mode, verbose=options.verbose)
  if options.cd:
    os.chdir(options.cd)

  if options.action == 't':
    tar.List()
    return 0

  if options.action == 'x':
    tar.Extract()
    return 0

  if options.action == 'c':
    for filepath in args:
      if not tar.Add(filepath, options.prefix):
        return -1
    tar.Close()
    return 0

  parser.error('Missing action c, t, or x.')
  return -1


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
