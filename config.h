#include <string>

class NotesConfig {
private:
    std::string notesFileName_;
    int windowWidth_;
    int windowHeight_;
    char hotkeyBase_;
    int hotkeyMod_;

    bool tryParseLine(const std::string& line);
public:
    NotesConfig(const std::string& notesFile);
    std::string getNotesFileName();
    int getWindowWidth();
    int getWindowHeight();
    int getHotkeyBase();
    int getHotkeyMod();
};