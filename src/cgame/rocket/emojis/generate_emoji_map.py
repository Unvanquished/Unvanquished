#!/usr/bin/python

import requests
import sys
import os

EMOJI_MAP_SRC = 'https://raw.githubusercontent.com/muan/emojilib/main/dist/emoji-en-US.json'
GOOGLE_EMOJI_SRC = 'https://github.com/googlefonts/emoji-metadata/raw/main/emoji_15_0_ordering.json'

CPLUSPLUS_CODE_HEADER="""
#include "emojimap.h"

#include <unordered_map>

const std::unordered_map<Str::StringRef, const char*> emojimap = {
"""

CPLUSPLUS_CODE_FOOTER="""};

Str::StringRef FindEmoji( Str::StringRef emoji )
{
	auto it = emojimap.find( emoji.c_str() );
	if ( it == emojimap.end() )
	{
		return "";
	}
	return it->second;
}

"""

def render_base(base):
    s = ''
    for codept in base:
        s += chr(codept)
    return s

def escapeq(s):
    return s.replace('"', '\\"')

def parse_google_emojis():
    req = requests.get(GOOGLE_EMOJI_SRC)
    if req.status_code != 200:
        print('Error fetching %s: %s' % (GOOGLE_EMOJI_SRC, req.text), file=sys.stderr)
        return None
    emojis = {}
    groups = req.json()
    for group in groups:
        for emoji in group['emoji']:
            if len(emoji['base']) > 1:
                continue
            for emoticon in emoji['emoticons']:
                if not emoticon in emoji:
                    emojis[escapeq(emoticon)] = render_base(emoji['base'])
            for short in emoji['shortcodes']:
                stripped = short[1:-1]
                if not stripped in emoji:
                    emojis[escapeq(stripped).lower()] = render_base(emoji['base'])
    return emojis


def parse_emojilib_emojis():
    req = requests.get(EMOJI_MAP_SRC)
    if req.status_code != 200:
        print('Error fetching %s: %s' % (EMOJI_MAP_SRC, req.text), file=sys.stderr)
        return None

    emojis = req.json()
    # Filter duplicates
    dedup = {}
    for codepoint, tags in emojis.items():
        for tag in tags:
            if not tag in emojis:
                dedup[tag] = codepoint
    return dedup


def main():
    emojis = parse_google_emojis()
    if not emojis:
        return -1
    print(CPLUSPLUS_CODE_HEADER)

    for tag, codepoint in emojis.items():
        print('\t{"%s", "%s"},' % (tag, codepoint))
    print(CPLUSPLUS_CODE_FOOTER)


if __name__ == '__main__':
    sys.exit(main())
