using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace AGS.Types
{
    using TranslationSourceNode = LinkedListNode<TranslationSourceLine>;

    public class TranslationSourceLine
    {
        public TranslationSourceLine(int line, string text)
        {
            Line = line;
            Text = text;
        }

        public int Line { get; set; }
        public string Text { get; set; }
    }

    public class TranslationSourceEntry
    {
        public TranslationSourceEntry(string key, TranslationSourceNode keyLine, TranslationSourceNode firstAnnotationLine = null)
        {
            Key = key;
            KeyLine = keyLine;
            ValueLine = keyLine.Next;
            FirstAnnotationLine = firstAnnotationLine;
        }

        public string Key { get; set; }
        public TranslationSourceNode KeyLine { get; set; }
        public TranslationSourceNode ValueLine { get; set; }
        public TranslationSourceNode FirstAnnotationLine { get; set; }
    }

    public class TranslationSourceOption
    {
        public TranslationSourceOption(string key, string value, TranslationSourceNode line)
        {
            Key = key;
            Value = value;
            Line = line;
        }

        public string Key { get; set; }
        public string Value { get; set; }
        public TranslationSourceNode Line { get; set; }
    }

    /// <summary>
    /// TranslationSource describes the exact contents of the translation source file.
    /// It serves as an intermediate stage between a file and Translation object, both
    /// when reading the file into memory, and when updating the file with the new
    /// game content.
    /// 
    /// The main purpose of this class is to be able to update existing translation
    /// source while keeping any user's comments, and removing or commenting out
    /// deprecated translation entries. These operations are impossible to achieve
    /// using Translation class alone, since Translation class contains a map of
    /// translation entries, and translation options, disconnected with their
    /// locations in the read file.
    /// </summary>
    public class TranslationSource
    {
        private const string NORMAL_FONT_TAG = "NormalFont";
        private const string NORMAL_FONT_COMMENT = "The normal font to use - DEFAULT or font number";
        private const string SPEECH_FONT_TAG = "SpeechFont";
        private const string SPEECH_FONT_COMMENT = "The speech font to use - DEFAULT or font number";
        private const string TEXT_DIRECTION_TAG = "TextDirection";
        private const string TEXT_DIRECTION_COMMENT = "Text direction - DEFAULT, LEFT or RIGHT";
        private const string AUTO_PARSERSAID_TAG = "AutoTranslateParserSaid";
        private const string AUTO_PARSERSAID_COMMENT = "Whether engine should translate Parser.Said strings automatically - ON or OFF";
        private const string ENCODING_TAG = "Encoding";
        private const string ENCODING_COMMENT = "Text encoding hint - ASCII or UTF-8";
        private const string LANGUAGE_TAG = "Language";
        private const string LANGUAGE_COMMENT = "Text language, use standard locale strings, like 'en', 'en_US', etc";
        private const string FONT_OVERRIDE_TAG = "Font";
        private const string TAG_DEFAULT = "DEFAULT";
        private const string TAG_DIRECTION_LEFT = "LEFT";
        private const string TAG_DIRECTION_RIGHT = "RIGHT";
        private const string TAG_ON = "ON";
        private const string TAG_OFF = "OFF";
        private const string ANNOTATE_OBSOLETE = "OBSOLETE";
        private const string ANNOTATE_PARSERWORD = "PARSERWORD";

        private string _encodingHint = null;
        private Encoding _encoding = Encoding.Default;
        private LinkedList<TranslationSourceLine> _lines = new LinkedList<TranslationSourceLine>();
        private Dictionary<string, TranslationSourceEntry> _entries = new Dictionary<string, TranslationSourceEntry>();
        private List<TranslationSourceEntry> _entryDuplicates = new List<TranslationSourceEntry>();
        private Dictionary<string, TranslationSourceOption> _options = new Dictionary<string, TranslationSourceOption>();
        private List<TranslationSourceOption> _optionDuplicates = new List<TranslationSourceOption>();
        private TranslationSourceNode _lastOptionBeforeFirstTranslationEntry = null;
        private List<TranslationSourceNode> _commentLines = new List<TranslationSourceNode>();

        public Encoding Encoding
        {
            get { return _encoding; }
            set
            {
                _encoding = value;
                _encodingHint = _encoding.WebName;
            }
        }

        public string EncodingHint
        {
            get { return _encodingHint; }
            set
            {
                _encodingHint = value;
                _encoding = Encoding.Default;
                if (!string.IsNullOrEmpty(value))
                {
                    if (string.Compare(_encodingHint, "UTF-8", true) == 0)
                        _encoding = new UTF8Encoding(false); // UTF-8 w/o BOM
                }
            }
        }

        public LinkedList<TranslationSourceLine> Lines
        {
            get { return _lines; }
            set { _lines = value; }
        }

        public Dictionary<string, TranslationSourceEntry> TranslationEntries
        {
            get { return _entries; }
            set { _entries = value; }
        }

        public Dictionary<string, TranslationSourceOption> TranslationOptions
        {
            get { return _options; }
            set { _options = value; }
        }

        public TranslationSource()
        {
        }

        public void ResetContents()
        {
            _lines.Clear();
            _entries.Clear();
            _entryDuplicates.Clear();
            _options.Clear();
            _optionDuplicates.Clear();
            _commentLines.Clear();
        }

        /// <summary>
        /// Loads translation source from a file.
        /// </summary>
        public bool Load(string filename, CompileMessages errors)
        {
            string tryEncoding = _encodingHint;
            bool result = false;
            using (var sr = new StreamReader(filename, _encoding))
            {
                result = LoadImpl(sr, errors);
            }
            if (!result && (_encodingHint != tryEncoding))
            {
                ResetContents();
                // Try again using encoding hint read from the translation file itself
                using (var sr = new StreamReader(filename, _encoding))
                {
                    result = LoadImpl(sr, errors);
                }
            }
            return result;
        }

        /// <summary>
        /// Loads translation source from a given stream.
        /// Fails and returns with error if the stream reader's encoding does not match the
        /// encoding hint found in the source data.
        /// </summary>
        public bool Load(StreamReader sr, CompileMessages errors)
        {
            return LoadImpl(sr, errors);
        }

        /// <summary>
        /// Saves translation source into the file.
        /// </summary>
        public void Save(string filename)
        {
            using (var sw = new StreamWriter(filename, false, _encoding))
            {
                Save(sw);
            }
        }

        /// <summary>
        /// Saves translation source into the given stream.
        /// </summary>
        public void Save(StreamWriter sw)
        {
            foreach (var line in _lines)
                sw.WriteLine(line.Text);
        }

        /// <summary>
        /// Saves translation directly into the file. This overwrites the file.
        /// </summary>
        public static void SaveTranslation(string filename, Translation translation)
        {
            using (var sw = new StreamWriter(filename, false, translation.Encoding))
            {
                SaveTranslationImpl(sw, translation);
            }
        }

        /// <summary>
        /// Saves translation directly into the given stream.
        /// </summary>
        public static void SaveTranslation(StreamWriter sw, Translation translation)
        {
            SaveTranslationImpl(sw, translation);
        }

        /// <summary>
        /// Parses the previously read translation source and applies
        /// parsed data into the Translation object.
        /// </summary>
        public void ApplyIntoTranslation(Translation translation)
        {
            translation.EncodingHint = EncodingHint;
            translation.ResetContents();

            foreach (var option in _options.Values)
            {
                ReadOption(translation, option.Key, option.Value);
            }

            foreach (var entry in _entries.Values)
            {
                translation.TranslatedLines.Add(entry.Key, entry.ValueLine.Value.Text);
                if (entry.FirstAnnotationLine != null)
                {
                    List<string> annotations = new List<string>();
                    for (var annLine = entry.FirstAnnotationLine; annLine != entry.KeyLine; annLine = annLine.Next)
                        annotations.Add(annLine.Value.Text);
                    translation.TranslatedEntryOptions[entry.Key] = CreateEntryOptions(annotations);
                }
            }
        }

        /// <summary>
        /// Merges the existing translation source (if one was previously
        /// loaded from a file or generated in any other way) with the
        /// contents of the given Translation object.
        /// </summary>
        public void MergeFromTranslation(Translation translation)
        {
            EncodingHint = translation.EncodingHint;
            // Merge options:
            // * New options appended to the list.
            // * Matching options have their values overwritten.
            WriteOption(NORMAL_FONT_TAG, WriteOptionalInt(translation.NormalFont), NORMAL_FONT_COMMENT);
            WriteOption(SPEECH_FONT_TAG, WriteOptionalInt(translation.SpeechFont), SPEECH_FONT_COMMENT);
            WriteOption(TEXT_DIRECTION_TAG, ((translation.RightToLeftText == true) ? TAG_DIRECTION_RIGHT : ((translation.RightToLeftText == null) ? TAG_DEFAULT : TAG_DIRECTION_LEFT)),
                TEXT_DIRECTION_COMMENT);
            WriteOption(ENCODING_TAG, (_encodingHint ?? "ASCII"), ENCODING_COMMENT);
            WriteOption(LANGUAGE_TAG, (translation.TextLanguage != null ? translation.TextLanguage.Replace('-', '_') : string.Empty),
                LANGUAGE_COMMENT);
            WriteOption(AUTO_PARSERSAID_TAG, (translation.AutoTranslateParserSaid ? TAG_ON : TAG_OFF), AUTO_PARSERSAID_COMMENT);

            // Process obsolete entries:
            // * Remove ones without translation
            // * Comment ones with translation (preserve user's work just in case)
            List<string> removeKeys = new List<string>();
            foreach (var oldEntry in _entries)
            {
                if (!translation.TranslatedLines.ContainsKey(oldEntry.Key))
                {
                    if (WriteObsoleteEntry(oldEntry.Value))
                        removeKeys.Add(oldEntry.Key);
                }
            }
            foreach (var removeKey in removeKeys)
                _entries.Remove(removeKey);

            // Merge translation entries
            // * New entries appended to the end of the document
            // * Matching entries have their translation overwritten, but only if it's not empty
            // * Entries that are present in TranslationSource, but are not matched in the
            //   Translation, on contrary, are either removed if translation is empty,
            //   or commented out if it's not empty.
            foreach (var entry in translation.TranslatedLines)
            {
                if (translation.TranslatedEntryOptions.ContainsKey(entry.Key))
                    WriteTranslationEntry(entry.Key, entry.Value, translation.TranslatedEntryOptions[entry.Key]);
                else
                    WriteTranslationEntry(entry.Key, entry.Value, null);
            }

            // TODO: also handle entry duplicates read from the file

            // After everything is merged, reenumerate Line indexes
            int lineIndex = 0;
            foreach (var line in _lines)
                line.Line = lineIndex++;
        }

        private bool LoadImpl(StreamReader sr, CompileMessages errors)
        {
            EncodingHint = sr.CurrentEncoding.WebName;
            TranslationSourceNode firstAnnotationLine = null;

            string line;
            int lineIndex = 0;
            for (line = sr.ReadLine(); line != null; line = sr.ReadLine(), ++lineIndex)
            {
                var lineNode = _lines.AddLast(new TranslationSourceLine(lineIndex, line));

                if (line.StartsWith("//"))
                {
                    // This is either a comment, a general option, or a entry annotation
                    if (line.Length > 2 && line[2] == '#')
                    {
                        // This is a option
                        var tagLine = line.Substring(3);
                        var keyValue = ParseKeyValue(tagLine);
                        var key = keyValue.Key;
                        var value = keyValue.Value;

                        if (key == ENCODING_TAG)
                        {
                            EncodingHint = value;
                            if (string.Compare(sr.CurrentEncoding.WebName, EncodingHint, true) != 0)
                            {
                                return false;
                            }
                        }

                        if (_options.ContainsKey(key))
                        {
                            _optionDuplicates.Add(new TranslationSourceOption(key, value, lineNode));
                            errors.Add(new CompileWarning($"Option '{key}' is present more than once. Any duplicates will be ignored."));
                        }
                        else
                        {
                            _options.Add(key, new TranslationSourceOption(key, value, lineNode));
                        }

                        firstAnnotationLine = null; // reset
                    }
                    else if (line.Length > 2 && line[2] == '$')
                    {
                        // This is a entry annotation
                        if (firstAnnotationLine == null)
                            firstAnnotationLine = lineNode;
                    }
                    else
                    {
                        // This is a pure comment
                        _commentLines.Add(lineNode);
                        firstAnnotationLine = null; // reset
                    }
                }
                else
                {
                    // This is potentially a translation entry
                    string originalText = line;
                    string translatedText = sr.ReadLine();
                    if (string.IsNullOrEmpty(originalText) && translatedText == null)
                    {
                        break;
                    }
                    else if (translatedText == null)
                    {
                        translatedText = string.Empty;
                    }

                    _lines.AddLast(new TranslationSourceLine(++lineIndex, translatedText));

                    var traEntry = new TranslationSourceEntry(originalText, lineNode);
                    if (firstAnnotationLine != null)
                    {
                        traEntry.FirstAnnotationLine = firstAnnotationLine;
                        firstAnnotationLine = null;
                    }

                    if (_entries.ContainsKey(originalText))
                    {
                        _entryDuplicates.Add(traEntry);
                        errors.Add(new CompileWarning($"Translation entry is present more than once. Any duplicates will be ignored. Entry text: \"{originalText}\""));
                    }
                    else
                    {
                        _entries.Add(originalText, traEntry);
                    }
                }
            }

            return true;
        }

        private static int? ReadOptionalInt(string textToParse)
        {
            int value;
            if (textToParse == TAG_DEFAULT || !int.TryParse(textToParse, out value))
            {
                return null;
            }
            return value;
        }

        private static string WriteOptionalInt(int? currentValue)
        {
            if (currentValue == null)
            {
                return TAG_DEFAULT;
            }
            return currentValue.Value.ToString();
        }

        // TODO: move to some utility module?
        private static KeyValuePair<string, string> ParseKeyValue(string line, char separator = '=')
        {
            int firstSep = line.IndexOf(separator);
            if (firstSep >= 0)
            {
                return new KeyValuePair<string, string>(
                    line.Substring(0, firstSep).Trim(),
                    line.Substring(firstSep + 1).Trim());
            }
            else
            {
                return new KeyValuePair<string, string>(line.Trim(), string.Empty);
            }
        }

        private static void ReadOption(Translation translation, string key, string value)
        {
            // NOTE: ENCODING_TAG is not included here, it's read during
            // source file parsing and stored in TranslationSource itself.

            if (key == NORMAL_FONT_TAG)
            {
                translation.NormalFont = ReadOptionalInt(value);
            }
            else if (key == SPEECH_FONT_TAG)
            {
                translation.SpeechFont = ReadOptionalInt(value);
            }
            else if (key == TEXT_DIRECTION_TAG)
            {
                string directionText = value;
                if (directionText == TAG_DIRECTION_LEFT)
                {
                    translation.RightToLeftText = false;
                }
                else if (directionText == TAG_DIRECTION_RIGHT)
                {
                    translation.RightToLeftText = true;
                }
                else
                {
                    translation.RightToLeftText = null;
                }
            }
            else if (key == AUTO_PARSERSAID_TAG)
            {
                if (value == TAG_ON)
                    translation.AutoTranslateParserSaid = true;
                else
                    translation.AutoTranslateParserSaid = false;
            }
            else if (key == LANGUAGE_TAG)
            {
                translation.TextLanguage = value;
            }
            else if (key.StartsWith(FONT_OVERRIDE_TAG))
            {
                int fontIndex = ParseFontN(key);
                if (fontIndex >= 0)
                {
                    if (!translation.FontOverrides.ContainsKey(fontIndex))
                    {
                        translation.FontOverrides.Add(fontIndex, ParseFontOverride(value));
                    }
                }
            }
        }

        /// <summary>
        /// Parses "FontN" kind of string, where N is a font's ID.
        /// </summary>
        private static int ParseFontN(string value)
        {
            int fontID;
            if (value.StartsWith("Font") && int.TryParse(value.Substring(4), out fontID))
                return fontID;
            return -1;
        }

        private static Font ParseFontOverride(string value)
        {
            // Format 1:
            //    FontN
            // Format 2:
            //    Property1=Value1;Property2=Value2;Property3=Value3;...
            int reFontNumber = ParseFontN(value);
            if (reFontNumber >= 0)
            {
                var font = new Font();
                font.ID = reFontNumber;
                return font;
            }
            else
            {
                var font = new Font();
                font.ID = -1; // mark it as not one of the game's font
                var options = value.Split(';').Select(s =>
                {
                    return ParseKeyValue(s);
                }).ToArray();
                foreach (var option in options)
                {
                    if (option.Key == "File")
                    {
                        font.ProjectFilename = option.Value;
                    }
                    else if (option.Key == "Size")
                    {
                        font.PointSize = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "SizeMultiplier")
                    {
                        font.SizeMultiplier = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "Outline")
                    {
                        if (option.Value == "NONE")
                        {
                            font.OutlineStyle = FontOutlineStyle.None;
                        }
                        else if (option.Value == "AUTO")
                        {
                            font.OutlineStyle = FontOutlineStyle.Automatic;
                        }
                        else
                        {
                            int outFontID = ParseFontN(option.Value);
                            if (outFontID >= 0)
                            {
                                font.OutlineStyle = FontOutlineStyle.UseOutlineFont;
                                font.OutlineFont = outFontID;
                            }
                        }
                    }
                    else if (option.Key == "AutoOutline")
                    {
                        if (option.Value == "SQUARED")
                        {
                            font.AutoOutlineStyle = FontAutoOutlineStyle.Squared;
                        }
                        else if (option.Value == "ROUND")
                        {
                            font.AutoOutlineStyle = FontAutoOutlineStyle.Rounded;
                        }
                    }
                    else if (option.Key == "AutoOutlineThickness")
                    {
                        font.AutoOutlineThickness = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "HeightDefinition")
                    {
                        if (option.Value == "NOMINAL")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.NominalHeight;
                        }
                        else if (option.Value == "REAL")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.PixelHeight;
                        }
                        else if (option.Value == "CUSTOM")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.CustomValue;
                        }
                    }
                    else if (option.Key == "CustomHeight")
                    {
                        font.CustomHeightValue = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "VerticalOffset")
                    {
                        font.VerticalOffset = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "LineSpacing")
                    {
                        font.LineSpacing = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "CharacterSpacing")
                    {
                        font.CharacterSpacing = option.Value.ParseIntOrDefault();
                    }
                }
                return font;
            }
        }

        private static TranslationEntryOptions CreateEntryOptions(List<string> annotations)
        {
            TranslationEntryOptions options = new TranslationEntryOptions();
            options.Metadata = new List<string>(annotations);

            // Parse for known annotations
            foreach (var annotation in annotations)
            {
                var keyValue = ParseKeyValue(annotation);

                if (keyValue.Key == ANNOTATE_OBSOLETE)
                {
                    options.IsObsolete = true;
                }
                else  if (keyValue.Key == ANNOTATE_PARSERWORD)
                {
                    options.ParserWordID = Utilities.ParseIntOrDefault(keyValue.Value, -1);
                }
            }

            return options;
        }

        /// <summary>
        /// Adds an option entry into the translation source.
        /// If such option already exists, then overwrites its value.
        /// If it does not, then inserts option, prepended with a comment.
        /// </summary>
        private void WriteOption(string key, string value, string comment)
        {
            if (_options.ContainsKey(key))
            {
                var optionEntry = _options[key];
                optionEntry.Value = value;
                optionEntry.Line.Value.Text = $"//#{key}={value}";
            }
            else
            {
                TranslationSourceNode insertAt = (_lastOptionBeforeFirstTranslationEntry != null) ?
                    _lastOptionBeforeFirstTranslationEntry.Next : _lines.First;
                var optionNode = _lines.AddBefore(insertAt, new TranslationSourceLine(insertAt.Value.Line, $"//#{key}={value}"));
                if (!string.IsNullOrEmpty(comment))
                    _lines.AddBefore(optionNode, new TranslationSourceLine(optionNode.Value.Line, $"// {comment}"));

                if (!string.IsNullOrEmpty(comment))
                    _commentLines.Add(optionNode.Previous);
                _options[key] = new TranslationSourceOption(key, value, optionNode);
                _lastOptionBeforeFirstTranslationEntry = optionNode;
            }
        }

        /// <summary>
        /// Adds a annotation to the given translation entry.
        /// If the annotation of the same key already exists, then overwrites it.
        /// Otherwise - inserts the new annotation right above the translation
        /// entry's key line.
        /// </summary>
        private void WriteAnnotation(TranslationSourceEntry entry, string annotation)
        {
            var newKeyValue = ParseKeyValue(annotation);
            bool annotationFound = false;
            if (entry.FirstAnnotationLine != null)
            {
                for (var annLine = entry.FirstAnnotationLine; annLine != entry.KeyLine; annLine = annLine.Next)
                {
                    var oldKeyValue = ParseKeyValue(annLine.Value.Text.Substring(3));
                    if (newKeyValue.Key == oldKeyValue.Key)
                    {
                        annLine.Value.Text = $"//${annotation}";
                        annotationFound = true;
                        break;
                    }
                }
            }

            // If annotation with such key was not found, then insert one
            // at the end of the annotations list (before the translation key)
            if (!annotationFound)
            {
                var insertAt = entry.KeyLine;
                _lines.AddBefore(insertAt, new TranslationSourceLine(insertAt.Value.Line, $"//${annotation}"));
            }
        }

        /// <summary>
        /// Adds a translation entry into the translation source.
        /// If the entry with such key already exists, then overwrites its translation line.
        /// If it does not, then appends this entry to the end of the list.
        /// 
        /// In case of existing entry, the metadata (annotations) will be merged in a similar
        /// fashion.
        /// </summary>
        private void WriteTranslationEntry(string key, string value, TranslationEntryOptions options)
        {
            if (_entries.ContainsKey(key))
            {
                var entry = _entries[key];
                entry.ValueLine.Value.Text = value;
                // Try merging metadata
                if (options != null)
                {
                    foreach (var annotation in options.Metadata)
                    {
                        WriteAnnotation(entry, annotation);
                    }
                }
            }
            else
            {
                TranslationSourceNode annotationLine = null;
                if (options != null)
                {
                    foreach (var annotation in options.Metadata)
                    {
                        var line = _lines.AddLast(new TranslationSourceLine(_lines.Count, annotation));
                        if (annotationLine == null)
                            annotationLine = line;
                    }
                }

                var keyLine = _lines.AddLast(new TranslationSourceLine(_lines.Count, key));
                _lines.AddLast(new TranslationSourceLine(_lines.Count, value));
                _entries.Add(key, new TranslationSourceEntry(key, keyLine, annotationLine));
            }
        }

        private bool WriteObsoleteEntry(TranslationSourceEntry entry)
        {
            if (string.IsNullOrEmpty(entry.ValueLine.Value.Text))
            {
                // No translation, so delete the obsolete entry altogether
                for (TranslationSourceNode line = entry.FirstAnnotationLine ?? entry.KeyLine, lastLine = entry.ValueLine.Next;
                        line != lastLine;)
                {
                    var remove = line;
                    line = line.Next;
                    _lines.Remove(remove);
                }
                return true;
            }
            else
            {
                // Has translation, so comment these out
                for (TranslationSourceNode line = entry.FirstAnnotationLine ?? entry.KeyLine, lastLine = entry.ValueLine.Next;
                        line != lastLine; line = line.Next)
                {
                    line.Value.Text = $"//{line.Value.Text}";
                }
                return false;
            }
        }

        private static string FormatFontOverride(KeyValuePair<int, Font> fontOverride)
        {
            StringBuilder sb = new StringBuilder();
            int fontIndex = fontOverride.Key;
            Font font = fontOverride.Value;
            sb.Append($"//#{FONT_OVERRIDE_TAG}{fontIndex}=");
            if (font.ID >= 0)
            {
                sb.Append($"Font{font.ID}");
            }
            else
            {
                // Only write non-default values. Unfortunately there's no way to know
                // which values user set in the original source file.
                sb.Append($"File={font.ProjectFilename};");
                if (font.PointSize > 0)
                    sb.Append($"Size={font.PointSize.ToString()};");
                if (font.SizeMultiplier > 1)
                    sb.Append($"SizeMultiplier={font.SizeMultiplier.ToString()};");

                if (font.OutlineStyle == FontOutlineStyle.Automatic)
                    sb.Append($"Outline=AUTO;");
                else if (font.OutlineStyle == FontOutlineStyle.UseOutlineFont)
                    sb.Append($"Outline=Font{font.OutlineFont};");

                if (font.OutlineStyle == FontOutlineStyle.Automatic)
                {
                    if (font.AutoOutlineStyle == FontAutoOutlineStyle.Rounded)
                        sb.Append($"AutoOutline=ROUND;");

                    sb.Append($"AutoOutlineThickness={font.AutoOutlineThickness};");
                }

                if (font.HeightDefinedBy == FontHeightDefinition.PixelHeight)
                    sb.Append($"HeightDefinition=REAL;");
                else if (font.HeightDefinedBy == FontHeightDefinition.CustomValue)
                    sb.Append($"HeightDefinition=CUSTOM;");

                if (font.HeightDefinedBy == FontHeightDefinition.CustomValue)
                {
                    sb.Append($"CustomHeight={font.CustomHeightValue};");
                }

                if (font.VerticalOffset != 0)
                    sb.Append($"VerticalOffset={font.VerticalOffset};");
                if (font.LineSpacing != 0)
                    sb.Append($"LineSpacing={font.LineSpacing};");
                if (font.CharacterSpacing != 0)
                    sb.Append($"CharacterSpacing={font.CharacterSpacing};");
            }
            return sb.ToString();
        }

        private static void SaveTranslationImpl(StreamWriter sw, Translation translation)
        {
            sw.WriteLine("// AGS TRANSLATION SOURCE FILE");
            sw.WriteLine("// Format is alternating lines with original game text and replacement");
            sw.WriteLine("// text. If you don't want to translate a line, just leave the following");
            sw.WriteLine("// line blank. Lines starting with '//' are comments - DO NOT translate");
            sw.WriteLine("// them. Special characters such as [ and %%s symbolise things within the");
            sw.WriteLine("// game, so should be left in an appropriate place in the message.");
            sw.WriteLine("// ");
            sw.WriteLine("// ** Translation settings are below");
            sw.WriteLine("// ** Leave them as \"DEFAULT\" to use the game settings");
            sw.WriteLine("// The normal font to use - DEFAULT or font number");
            sw.WriteLine($"//#{NORMAL_FONT_TAG}={WriteOptionalInt(translation.NormalFont)}");
            sw.WriteLine("// The speech font to use - DEFAULT or font number");
            sw.WriteLine($"//#{SPEECH_FONT_TAG}={WriteOptionalInt(translation.SpeechFont)}");
            sw.WriteLine("// Text direction - DEFAULT, LEFT or RIGHT");
            sw.WriteLine($"//#{TEXT_DIRECTION_TAG}={((translation.RightToLeftText == true) ? TAG_DIRECTION_RIGHT : ((translation.RightToLeftText == null) ? TAG_DEFAULT : TAG_DIRECTION_LEFT))}");
            sw.WriteLine("// Text encoding hint - ASCII or UTF-8");
            sw.WriteLine($"//#{ENCODING_TAG}={(translation.EncodingHint ?? "ASCII")}");
            sw.WriteLine("// Text language, use standard locale strings, like 'en', 'en_US', etc");
            sw.WriteLine($"//#{LANGUAGE_TAG}={(translation.TextLanguage != null ? translation.TextLanguage.Replace('-', '_') : string.Empty)}");
            sw.WriteLine("// Whether engine should translate Parser.Said strings automatically - ON or OFF");
            sw.WriteLine($"//#{AUTO_PARSERSAID_TAG}={(translation.AutoTranslateParserSaid ? TAG_ON : TAG_OFF)}");
            if (translation.FontOverrides.Count != 0)
            {
                foreach (var fontOverride in translation.FontOverrides)
                {
                    sw.WriteLine(FormatFontOverride(fontOverride));
                }
            }
            sw.WriteLine("//  ");
            sw.WriteLine("// ** REMEMBER, WRITE YOUR TRANSLATION IN THE EMPTY LINES, DO");
            sw.WriteLine("// ** NOT CHANGE THE EXISTING TEXT.");

            foreach (var entry in translation.TranslatedLines)
            {
                TranslationEntryOptions entryOptions;
                if (translation.TranslatedEntryOptions.TryGetValue(entry.Key, out entryOptions))
                {
                    foreach (var a in entryOptions.Metadata)
                        sw.WriteLine($"//${a}");
                }

                sw.WriteLine(entry.Key);
                sw.WriteLine(entry.Value);
            }
        }
    }
}
