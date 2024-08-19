This is a HACKED version of the AGS engine, made for the purpose of aiding people who translate older games without an access to the game source.

It contains a number of additional commands and options that are not part of the official AGS, and are meant as a last resort when official features are not enough to make a game's translation work.

1. Translating Parser's dictionary.
following option has to be added to the game's config:
```
[hack]
parser_dic=<path to file>
```
If this file does *not* exist will generate a list of words in a dictionary, one word per line.
If this file *does* exist will read it back and replace the dictionary words with the ones read from the file.

2. Automatically translate input parameters in Parser script commands.
following option has to be added to the game's config:
```
[hack]
tra_parsersaid=1
```
This will force engine to translate input parameters to `Parser.Said()` and `Parser.FindWordID()` commands in script.

3. Pretend that the game is not using translation.
following option has to be added to the game's config:
```
[hack]
stealth_tra=1
```
This might be useful if the game has different script behavior for when using translation, which you want to avoid.

4. When translating, also check for keys without voice tokens ("&N").
following option has to be added to the game's config:
```
[hack]
tra_trynovoice=1
```
Suppose there's a sentence "&10 Hello" in the game script, but the translation file has "Hello" as a key for some reason.
This option will make engine try both keys "&10 Hello" and "Hello" when looking for a translation.

5. Enforce translation of ListBoxes items in game.
following option has to be added to the game's config:
```
[hack]
tra_listbox=1
```

6. Enforce "debug mode" setting in game.
following option has to be added to the game's config:
```
[hack]
debug_mode=1
```

7. Force fonts auto-outlining (purely visual effect).
following option has to be added to the game's config:
```
[hack]
font_autooutline=<comma separated list of font IDs>
```

8. Extract old format messages from the game data.
following command line argument is used:
```
ags.exe gamedata --extract-messages <DIR>
```
This will extract:
  * old-style global messages;
  * old-style room messages, from all the rooms found in game package;
  * old-style dialog lines.

