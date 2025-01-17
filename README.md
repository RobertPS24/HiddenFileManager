# HiddenFileManager
This application is a utility for searching and managing hidden files in a user-specified directory. The graphical interface is implemented as a Windows dialog and offers the following functionalities:

1. **Directory Selection**  
   The user can select a directory using a file system explorer dialog, activated by the "Select Directory" button.  
   After selection, the application displays the content of the chosen directory in the "EDITTEXT" field (marked as `IDC_EDIT_DIR`).

2. **Hidden File Detection**  
   The application recursively scans the selected directory for files with the hidden attribute (`FILE_ATTRIBUTE_HIDDEN`).  
   Detected hidden files are added to a unique list and displayed in another text field (`IDC_EDIT_HIDDEN`).

3. **Hidden File Moving**  
   The user receives a notification for confirming the move of hidden files.  
   Detected hidden files are moved to a subdirectory named `TempHidden`, created within the currently selected directory.  
   If a file move operation fails, an error message with the corresponding error code is displayed.

4. **Automatic Periodic Update**  
   The application uses a timer configured to 5 seconds (`WM_TIMER`), which triggers the automatic search and management of hidden files.

5. **Automatic State Reset**  
   When the selected directory changes, the application's state is reset: the list of hidden files is cleared, and any file move operations are revoked.
