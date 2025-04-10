using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
using AGS.Editor.TextProcessing;

namespace AGS.Editor.Components
{
    class CharactersComponent : BaseComponentWithFolders<Character, CharacterFolder>
    {
        private const string CHARACTERS_COMMAND_ID = "Characters";
        private const string COMMAND_NEW_ITEM = "NewCharacter";
        private const string COMMAND_IMPORT = "ImportCharacter";
        private const string COMMAND_DELETE_ITEM = "DeleteCharacter";
        private const string COMMAND_EXPORT = "ExportCharacter";
        private const string COMMAND_CHANGE_ID = "ChangeCharacterID";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string ICON_KEY = "CharactersIcon";
        
        private const string CHARACTER_EXPORT_FILE_FILTER = "AGS 3.1+ exported characters (*.chr)|*.chr|AGS 2.72/3.0 exported characters (*.cha)|*.cha";
        private const string CHARACTER_IMPORT_FILE_FILTER = "AGS exported characters (*.chr; *.cha)|*.chr;*.cha|AGS 3.1+ exported characters (*.chr)|*.chr|AGS 2.72/3.0 exported characters (*.cha)|*.cha";
        private const string NEW_CHARACTER_FILE_EXTENSION = ".chr";

        private Dictionary<Character, ContentDocument> _documents;
        private Character _itemRightClicked = null;

        public event EventHandler<CharacterIDChangedEventArgs> OnCharacterIDChanged;
        public event EventHandler<CharacterRoomChangedEventArgs> OnCharacterRoomChanged;

        public CharactersComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, CHARACTERS_COMMAND_ID)
        {
            _documents = new Dictionary<Character, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("charactr.ico"));
            _guiController.RegisterIcon("CharacterIcon", Resources.ResourceManager.GetIcon("charactr-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Characters", ICON_KEY);
            _agsEditor.CheckGameScripts += ScanAndReportMissingInteractionHandlers;
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Characters; }
        }

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {                
                Character newItem = new Character();
                newItem.ID = _agsEditor.CurrentGame.RootCharacterFolder.GetAllItemsCount();
                newItem.ScriptName = _agsEditor.GetFirstAvailableScriptName("cChar");
                newItem.RealName = "New character";
                newItem.StartingRoom = -1;
                string newNodeID = AddSingleItem(newItem);
                _guiController.ProjectTree.SelectNode(this, newNodeID);
				ShowOrAddPane(newItem);
            }
            else if (controlID == COMMAND_IMPORT)
            {
                string fileName = _guiController.ShowOpenFileDialog("Select character to import...", CHARACTER_IMPORT_FILE_FILTER);
                if (fileName != null)
                {
                    ImportCharacter(fileName);
                }
            }
            else if (controlID == COMMAND_EXPORT)
            {
                string fileName = _guiController.ShowSaveFileDialog("Export character as...", CHARACTER_EXPORT_FILE_FILTER);
                if (fileName != null)
                {
                    ExportCharacter(_itemRightClicked, fileName);
                }
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsages.Find(null, _itemRightClicked.ScriptName);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this character? Doing so could break any scripts that refer to characters by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    Character c = _itemRightClicked;
                    // For a lack of better solution at the moment, pretend that character leaves current room before being deleted
                    int oldRoom = c.StartingRoom;
                    c.StartingRoom = -1;
                    OnCharacterRoomChanged?.Invoke(this, new CharacterRoomChangedEventArgs(c, oldRoom));
                    DeleteSingleItem(_itemRightClicked);
                }
            }
            else if (controlID == COMMAND_CHANGE_ID)
            {
                int oldNumber = _itemRightClicked.ID;
                int newNumber = Factory.GUIController.ShowChangeObjectIDDialog("Character", oldNumber, 0, _items.Count - 1);
                if (newNumber < 0)
                    return;
                foreach (var obj in _items)
                {
                    if (obj.Value.ID == newNumber)
                    {
                        obj.Value.ID = oldNumber;
                        break;
                    }
                }
                _itemRightClicked.ID = newNumber;
                GetFlatList().Swap(oldNumber, newNumber);
                OnItemIDOrNameChanged(_itemRightClicked, false);
                OnCharacterIDChanged?.Invoke(this, new CharacterIDChangedEventArgs(_itemRightClicked, oldNumber));
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                Character chosenItem = _items[controlID];
                ShowOrAddPane(chosenItem);
            }
        }

        private void DeleteCharacter(Character character)
        {
            int removingID = character.ID;
            foreach (Character item in _agsEditor.CurrentGame.RootCharacterFolder.AllItemsFlat)
            {
                if (item.ID > removingID)
                {
                    item.ID--;
                    OnCharacterIDChanged?.Invoke(this, new CharacterIDChangedEventArgs(item, item.ID + 1));
                }
            }

            ContentDocument document;
            if (_documents.TryGetValue(character, out document))
            {
                _guiController.RemovePaneIfExists(document);
                _documents.Remove(character);
            }            
        }

		private void ShowOrAddPane(Character chosenItem)
		{
            ContentDocument document;
			if (!_documents.TryGetValue(chosenItem, out document)
                || document.Control.IsDisposed)
			{
                document = new ContentDocument(new CharacterEditor(chosenItem),
                    chosenItem.WindowTitle, this, ICON_KEY,
                    ConstructPropertyObjectList(chosenItem));
				_documents[chosenItem] = document;
				document.SelectedPropertyGridObject = chosenItem;
			}
            document.TreeNodeID = GetNodeID(chosenItem);
			_guiController.AddOrShowPane(document);
		}

        public override IList<string> GetManagedScriptElements()
        {
            return new string[] { "Character" };
        }

        public override bool ShowItemPaneByName(string name)
        {
            IList<Character> characters = GetFlatList();
            foreach(Character c in characters)
            {
                if(c.ScriptName == name)
                {
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(c));
                    ShowOrAddPane(c);
                    return true;
                }
            }
            return false;
        }

        public void ShowPlayerCharacter()
        {
            Character player = Factory.AGSEditor.CurrentGame.PlayerCharacter;
            _guiController.ProjectTree.SelectNode(this, GetNodeID(player));
            ShowOrAddPane(player);
        }

        private void OnItemIDOrNameChanged(Character item, bool name_only)
        {
            // Refresh tree, property grid and open windows
            if (name_only)
                ChangeItemLabel(GetNodeID(item), GetNodeLabel(item));
            else
                RePopulateTreeView(GetNodeID(item)); // currently this is the only way to update tree item ids

            foreach (ContentDocument doc in _documents.Values)
            {
                var docItem = ((CharacterEditor)doc.Control).ItemToEdit;
                doc.Name = docItem.WindowTitle;
                _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(docItem), doc, docItem);
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            Character itemBeingEdited = ((CharacterEditor)_guiController.ActivePane.Control).ItemToEdit;

            if (propertyName == Character.PROPERTY_NAME_SCRIPTNAME)
            {
                bool nameInUse = _agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName, itemBeingEdited);
                if (itemBeingEdited.ScriptName.StartsWith("c") && itemBeingEdited.ScriptName.Length > 1)
                {
                    nameInUse |= _agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName.Substring(1).ToUpperInvariant(), itemBeingEdited);
                }
                if (nameInUse)
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    itemBeingEdited.ScriptName = (string)oldValue;
                }
                else
                {
                    OnItemIDOrNameChanged(itemBeingEdited, true);
                    OnCharacterIDChanged?.Invoke(this, new CharacterIDChangedEventArgs(itemBeingEdited, itemBeingEdited.ID));
                }
            }
            else if (propertyName == Character.PROPERTY_NAME_STARTINGROOM)
            {
                if (OnCharacterRoomChanged != null)
                {
                    int oldRoom = (int)oldValue;
                    OnCharacterRoomChanged(this, new CharacterRoomChangedEventArgs(itemBeingEdited, oldRoom));
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))
            {
                int charID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootCharacterFolder.FindCharacterByID(charID, true);
                menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change character ID", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this character", null));
                menu.Add(new MenuCommand(COMMAND_EXPORT, "Export character...", null));
                menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + _itemRightClicked.ScriptName, null));
            }
            return menu;
        }

        // Synchronize open character documents with Views
        public void UpdateCharacterViews()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                ((CharacterEditor)doc.Control).UpdateViewPreview();
            }
        }

        public override void RefreshDataFromGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();

            RePopulateTreeView();
        }

        private void ExportCharacter(Character character, string fileName)
        {
            try
            {
                if (fileName.ToLower().EndsWith(NEW_CHARACTER_FILE_EXTENSION))
                {
                    ImportExport.ExportCharacterNewFormat(character, fileName, _agsEditor.CurrentGame);
                }
                else
                {
                    ImportExport.ExportCharacter272(character, fileName, _agsEditor.CurrentGame);
                }
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowMessage("An error occurred exporting the character file. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void ImportCharacter(string fileName)
        {
            try
            {
                Character character;

                if (fileName.ToLower().EndsWith(NEW_CHARACTER_FILE_EXTENSION))
                {
                    character = ImportExport.ImportCharacterNew(fileName, _agsEditor.CurrentGame);
                }
                else
                {
                    character = ImportExport.ImportCharacter272(fileName, _agsEditor.CurrentGame);
                }

                character.ID = _agsEditor.CurrentGame.RootCharacterFolder.GetAllItemsCount();
                AddSingleItem(character);
                // Pretend that character has just changed into the new room
                OnCharacterRoomChanged?.Invoke(this, new CharacterRoomChangedEventArgs(character, -1));
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowMessage("An error occurred importing the character file. The error was: " + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
            }
        }        

        private Dictionary<string, object> ConstructPropertyObjectList(Character item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.PropertyGridTitle, item);
            return list;
        }

        protected override CharacterFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootCharacterFolder;
        }

        protected override IList<Character> GetFlatList()
        {
            return _agsEditor.CurrentGame.CharacterFlatList;
        }

        private string GetNodeID(Character item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        private string GetNodeLabel(Character item)
        {
            return item.ID.ToString() + ": " + item.ScriptName;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(Character item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), GetNodeLabel(item), "CharacterIcon");            
            return treeItem;
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New character", null));
            menu.Add(new MenuCommand(COMMAND_IMPORT, "Import character...", null));  
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            // No more commands in this menu
        }

        protected override bool CanFolderBeDeleted(CharacterFolder folder)
        {
            return true;
        }

        protected override void DeleteResourcesUsedByItem(Character item)
        {
            DeleteCharacter(item);
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its characters?" + Environment.NewLine + Environment.NewLine + "If any of the characters are referenced in code by their number it could cause crashes in the game.";
        }

        private void ScanAndReportMissingInteractionHandlers(GenericMessagesArgs args)
        {
            var errors = args.Messages;
            foreach (Character c in _agsEditor.CurrentGame.Characters)
            {
                var funcs = _agsEditor.Tasks.FindInteractionHandlers(c.ScriptName, c.Interactions, true);
                if (funcs == null || funcs.Length == 0)
                    continue;

                for (int i = 0; i < funcs.Length; ++i)
                {
                    bool has_interaction = !string.IsNullOrEmpty(c.Interactions.ScriptFunctionNames[i]);
                    bool has_function = funcs[i].HasValue;
                    // If we have an assigned interaction function, but the function is not found - report a missing warning
                    if (has_interaction && !has_function)
                    {
                        errors.Add(new CompileWarningWithGameObject($"Character ({c.ID}) {c.ScriptName}'s event {c.Interactions.Schema.FunctionSuffixes[i]} function \"{c.Interactions.ScriptFunctionNames[i]}\" not found in script {c.Interactions.ScriptModule}.",
                            "Character", c.ScriptName, true));
                    }
                    // If we don't have an assignment, but has a similar function - report a possible unlinked function
                    else if (!has_interaction && has_function)
                    {
                        errors.Add(new CompileWarningWithGameObject($"Function \"{funcs[i].Value.Name}\" looks like an event handler, but is not linked on Character ({c.ID}) {c.ScriptName}'s Event pane",
                            "Character", c.ScriptName, funcs[i].Value.ScriptName, funcs[i].Value.Name, funcs[i].Value.LineNumber));
                    }
                }
            }
        }
    }
}
