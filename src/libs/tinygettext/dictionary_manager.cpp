//  tinygettext - A gettext replacement that works directly on .po files
//  Copyright (C) 2006 Ingo Ruhnke <grumbel@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "dictionary_manager.hpp"

#include <memory>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <algorithm>

#include "log_stream.hpp"
#include "po_parser.hpp"

namespace tinygettext {

static bool has_suffix(const std::string& lhs, const std::string rhs)
{
  if (lhs.length() < rhs.length())
    return false;
  else
    return lhs.compare(lhs.length() - rhs.length(), rhs.length(), rhs) == 0;
}

DictionaryManager::DictionaryManager(const std::string& charset_) :
  dictionaries(),
  search_path(),
  charset(charset_),
  use_fuzzy(true),
  current_language(),
  current_dict(0),
  empty_dict()
{
}

DictionaryManager::~DictionaryManager()
{
  for(Dictionaries::iterator i = dictionaries.begin(); i != dictionaries.end(); ++i)
  {
    delete i->second;
  }
}

void
DictionaryManager::clear_cache()
{
  for(Dictionaries::iterator i = dictionaries.begin(); i != dictionaries.end(); ++i)
  {
    delete i->second;
  }
  dictionaries.clear();

  current_dict = 0;
}

Dictionary&
DictionaryManager::get_dictionary()
{
  if (current_dict)
  {
    return *current_dict; 
  }
  else
  {
    if (current_language)
    {
      current_dict = &get_dictionary(current_language);
      return *current_dict;
    }
    else
    {
      return empty_dict;
    }
  }
}

Dictionary&
DictionaryManager::get_dictionary(const Language& language)
{
  //log_debug << "Dictionary for language \"" << spec << "\" requested" << std::endl;
  //log_debug << "...normalized as \"" << lang << "\"" << std::endl;
  assert(language);

  Dictionaries::iterator i = dictionaries.find(language); 
  if (i != dictionaries.end())
  {
    return *i->second;
  }
  else // Dictionary for languages lang isn't loaded, so we load it
  {
    //log_debug << "get_dictionary: " << lang << std::endl;
    Dictionary* dict = new Dictionary(charset);

    dictionaries[language] = dict;
    return *dict;
  }

}

std::set<Language>
DictionaryManager::get_languages()
{
  std::set<Language> languages;

  for (std::map<Language,Dictionary*>::iterator p = dictionaries.begin(); p != dictionaries.end(); ++p)
  {
    languages.insert(p->first);
  }
  return languages;
}

void
DictionaryManager::set_language(const Language& language)
{
  if (current_language != language)
  {
    current_language = language;
    current_dict     = 0;
  }
}

Language
DictionaryManager::get_language() const
{
  return current_language;
}

void
DictionaryManager::set_charset(const std::string& charset_)
{
  clear_cache(); // changing charset invalidates cache
  charset = charset_;
}

void
DictionaryManager::set_use_fuzzy(bool t)
{
  clear_cache();
  use_fuzzy = t;
}

bool
DictionaryManager::get_use_fuzzy() const
{
  return use_fuzzy;
}

void
DictionaryManager::add_directory(const std::string& pathname)
{
  clear_cache(); // adding directories invalidates cache
  search_path.push_back(pathname);
}

void 
DictionaryManager::add_po(const std::string& name, std::istream& in, const Language& lang)
{
  Dictionary *dict = new Dictionary(charset);
  POParser::parse( name, in, *dict ); 
  dictionaries[lang] = dict;
}
  
} // namespace tinygettext

/* EOF */
