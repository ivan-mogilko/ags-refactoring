using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Translation
    {
        public const string TRANSLATION_SOURCE_FILE_EXTENSION = ".trs";
        public const string TRANSLATION_COMPILED_FILE_EXTENSION = ".tra";

        private string _name;
        private string _fileName;
        private bool _modified;
        private int? _normalFont = null;
        private int? _speechFont = null;
        private bool? _rightToLeftText = null;
        private bool _autoTranslateParserSaid = false;
        private string _encodingHint;
        private Encoding _encoding;
        private string _language = string.Empty;
        private Dictionary<int, Font> _fontOverrides = new Dictionary<int, Font>();
        private Dictionary<string, string> _translatedLines;
        private Dictionary<string, TranslationEntryOptions> _entryOptions;

        public Translation(string name)
        {
            Name = name;
            EncodingHint = "UTF-8";
        }

        public string Name
        {
            get { return _name; }
            set { _name = value; _fileName = _name + TRANSLATION_SOURCE_FILE_EXTENSION; }
        }
        
        public string FileName
        {
            get { return _fileName; }
        }

        public string CompiledFileName
        {
            get { return _name + TRANSLATION_COMPILED_FILE_EXTENSION; }
        }

        public Dictionary<string, string> TranslatedLines
        {
            get { return _translatedLines; }
            set { _translatedLines = value; }
        }

        public Dictionary<string, TranslationEntryOptions> TranslatedEntryOptions
        {
            get { return _entryOptions; }
            set { _entryOptions = value; }
        }

        public int? NormalFont
        {
            get { return _normalFont; }
            set { _normalFont = value; }
        }

        public int? SpeechFont
        {
            get { return _speechFont; }
            set { _speechFont = value; }
        }

        public bool? RightToLeftText
        {
            get { return _rightToLeftText; }
            set { _rightToLeftText = value; }
        }

        public bool AutoTranslateParserSaid
        {
            get { return _autoTranslateParserSaid; }
            set { _autoTranslateParserSaid = value; }
        }

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

        public string TextLanguage
        {
            get { return _language; }
            set { _language = value; }
        }

        /// <summary>
        /// Contains definitions of font overrides provided for the given font indexes;
        /// these have to be used when this translation is loaded at runtime.
        /// If the Font object contains a real ID, that means that the override
        /// should replace a font with another existing font (so long as it's valid).
        /// Otherwise, this instructs that the new font should be generated with
        /// the given set of properties.
        /// </summary>
        public Dictionary<int, Font> FontOverrides
        {
            get { return _fontOverrides; }
            set { _fontOverrides = value; }
        }

        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        public Translation(XmlNode node)
        {
            this.Name = SerializeUtils.GetElementString(node, "Name");
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            _encodingHint = null;
            _encoding = Encoding.Default;
            try
            {
                LoadData();
            }
            catch (Exception)
            {
                _translatedLines.Clear(); // clear on failure
            }
        }

        public void ResetContents()
        {
            _modified = true;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            _autoTranslateParserSaid = false;
            _language = string.Empty;
            _fontOverrides.Clear();
            _translatedLines.Clear();
            _entryOptions.Clear();
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Translation");
            writer.WriteElementString("Name", _name);
            writer.WriteEndElement();
        }

        public void SaveData()
        {
            TranslationSource.SaveTranslation(FileName, this);
            this.Modified = false;
        }

        /// <summary>
        /// Loads translation data from the source file (TRS).
        /// Throws IO exceptions.
        /// </summary>
        public void LoadData()
        {
            CompileMessages errors = new CompileMessages();
            LoadDataImpl(errors);
        }

        /// <summary>
        /// Loads translation data from the source file (TRS).
        /// Suppresses exceptions and returns error messages.
        /// </summary>
        public CompileMessages TryLoadData()
        {
            CompileMessages errors = new CompileMessages();
            try
            {
                LoadDataImpl(errors);
            }
            catch (Exception e)
            {
                errors.Add(new CompileError(string.Format("Failed to load translation from {0}: \n{1}", FileName, e.Message)));
                ResetContents();
            }
            return errors;
        }

        private bool LoadDataImpl(CompileMessages errors)
        {
            _fontOverrides = new Dictionary<int, Font>();
            _translatedLines = new Dictionary<string, string>();
            _entryOptions = new Dictionary<string, TranslationEntryOptions>();
            List<string> annotateNextLine = new List<string>();
            string old_encoding = _encodingHint;

            TranslationSource traSource = new TranslationSource();
            if (!traSource.Load(FileName, errors))
                return false;
            traSource.ApplyIntoTranslation(this);
            return true;
        }
    }
}
