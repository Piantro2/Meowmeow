/*
 * SYMBOLIC NUMBLER GAME - VERSI FINAL GABUNGAN
 *
 * FITUR:
 * - Gameplay tebak angka dengan seni ASCII.
 * - Petunjuk panah "lebih tinggi" atau "lebih rendah".
 * - Sistem Papan Peringkat (Leaderboard) yang disimpan ke file.
 * - Teks antarmuka dalam Bahasa Indonesia.
 * - Kompatibel dengan Windows, Linux, dan macOS.
 */

// BAGIAN HEADER INCLUDES
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <cctype>
#include <map>
#include <cstdlib>
#include <sstream>
#include <limits>
#include <fstream> // Untuk file I/O (papan peringkat)

// Header spesifik platform untuk input langsung
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// Struktur untuk menyimpan data di papan peringkat
struct ScoreEntry {
    string playerName;
    int score;
    int digits;
    int attempts;
};

// Deklarasi fungsi pembantu
int getIntegerInput(const string& prompt, int min, int max);

/*
 * DEFINISI KELAS UTAMA PERMAINAN
 */
class SymbolicNumbler {
private:
    // VARIABEL STATUS PERMAINAN
    string target;
    vector<string> guesses;
    vector<string> results;
    int maxAttempts;
    int currentAttempt;
    int digits;

    // DATA PAPAN PERINGKAT
    vector<ScoreEntry> leaderboard;
    const string leaderboardFilename = "symbolic_leaderboard.txt";

    // KONSTANTA WARNA ANSI
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* RESET = "\033[0m";

    int BORDER_WIDTH = 80;

    // PEMETAAN SIMBOL DIGIT ASCII
    const map<char, string> digitSymbols = {
        {'0', " ##### \n##   ##\n##   ##\n##   ##\n##   ##\n##   ##\n ##### "},
        {'1', "   ##  \n ####  \n   ##  \n   ##  \n   ##  \n   ##  \n#######"},
        {'2', " ##### \n##   ##\n     ##\n ##### \n##     \n##     \n#######"},
        {'3', " ##### \n##   ##\n     ##\n ##### \n     ##\n##   ##\n ##### "},
        {'4', "##   ##\n##   ##\n##   ##\n#######\n     ##\n     ##\n     ##"},
        {'5', "#######\n##     \n##     \n###### \n     ##\n##   ##\n ##### "},
        {'6', " ##### \n##   ##\n##     \n###### \n##   ##\n##   ##\n ##### "},
        {'7', "#######\n     ##\n    ## \n   ##  \n  ##   \n ##    \n ##    "},
        {'8', " ##### \n##   ##\n##   ##\n ##### \n##   ##\n##   ##\n ##### "},
        {'9', " ##### \n##   ##\n##   ##\n ######\n     ##\n##   ##\n ##### "}
    };

    // SIMBOL PANAH ASCII UNTUK PETUNJUK
    const map<string, string> arrowSymbols = {
        {"UP",
         "   ##   \n"
         "  ####  \n"
         " ###### \n"
         "   ##   \n"
         "   ##   \n"
         "   ##   \n"
         "   ##   "},
        {"DOWN",
         "   ##   \n"
         "   ##   \n"
         "   ##   \n"
         "   ##   \n"
         " ###### \n"
         "  ####  \n"
         "   ##   "}
    };

    // Menghitung panjang visual (mengabaikan kode warna ANSI)
    int getVisualLength(const string& text) {
        int length = 0;
        bool inColorCode = false;
        for (char c : text) {
            if (c == '\033') {
                inColorCode = true;
            } else if (inColorCode && c == 'm') {
                inColorCode = false;
            } else if (!inColorCode) {
                length++;
            }
        }
        return length;
    }

    // Menghitung skor berdasarkan sisa percobaan dan jumlah digit
    int calculateScore() {
        return (maxAttempts - currentAttempt + 1) * digits * 50;
    }

public:
    // FUNGSI UTILITAS (STATIC)
#ifdef _WIN32
    static char getChar() { return _getch(); }
    static void clearScreen() { system("cls"); }
#else
    static char getChar() {
        char buf = 0;
        struct termios old = {0};
        fflush(stdout);
        tcgetattr(0, &old);
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &old);
        if (read(0, &buf, 1) < 0) perror("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        tcsetattr(0, TCSADRAIN, &old);
        return buf;
    }
    static void clearScreen() { cout << "\033[2J\033[1;1H"; }
#endif

    // FUNGSI JEDA
    void pauseForInput() {
        cout << "\nTekan tombol apa saja untuk melanjutkan...";
        getChar();
    }

    // KONSTRUKTOR
    SymbolicNumbler(int initial_digits = 4, int initial_attempts = 6) {
        startNewGame(initial_digits, initial_attempts);
        loadLeaderboard();
    }

    // Memulai atau mereset permainan dengan pengaturan baru
    void startNewGame(int newDigits, int newAttempts) {
        digits = newDigits;
        maxAttempts = newAttempts;
        currentAttempt = 0;
        guesses.clear();
        results.clear();
        BORDER_WIDTH = max(80, digits * 10 + 20);
        generateTarget(digits);
    }

    // Membuat angka target acak
    void generateTarget(int numDigits) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 9);
        target = "";
        for (int i = 0; i < numDigits; i++) {
            target += to_string(dis(gen));
        }
    }

    // FUNGSI TAMPILAN (BORDER, TEKS, DLL.)
    void printBorder(bool top = false, bool bottom = false) {
        cout << CYAN;
        char borderChar = (top || bottom) ? '=' : '-';
        cout << "+";
        for (int i = 0; i < BORDER_WIDTH - 2; i++) cout << borderChar;
        cout << "+";
        cout << RESET << "\n";
    }

    void printInBorder(const string& text, bool center = true) {
        cout << CYAN << "|" << RESET;
        int visibleLength = getVisualLength(text);
        int padding = BORDER_WIDTH - 2 - visibleLength;
        if (center) {
            int leftPad = padding / 2;
            int rightPad = padding - leftPad;
            cout << string(leftPad, ' ') << text << string(rightPad, ' ');
        } else {
            cout << " " << text << string(padding - 1, ' ');
        }
        cout << CYAN << "|" << RESET << "\n";
    }

    void printEmptyBorderLine() {
        cout << CYAN << "|" << string(BORDER_WIDTH - 2, ' ') << "|" << RESET << "\n";
    }

    // Menampilkan angka dalam bentuk seni ASCII
    void displaySymbolicNumber(const string& number, const string& feedback = "", bool isInput = false) {
        vector<vector<string>> symbolLines(7);
        for (char digitChar : number) {
            if (!isdigit(digitChar)) continue;

            stringstream ss(digitSymbols.at(digitChar));
            string line;
            vector<string> lines;
            while (getline(ss, line, '\n')) lines.push_back(line);

            string color = RESET;
            if (isInput) color = MAGENTA;
            else if (!feedback.empty() && (symbolLines[0].size() < feedback.length())) {
                switch (feedback[symbolLines[0].size()]) {
                    case 'G': color = GREEN; break;
                    case 'Y': color = YELLOW; break;
                    case 'X': color = RED; break;
                }
            }

            for (int j = 0; j < 7; j++) {
                symbolLines[j].push_back(color + lines[j] + RESET);
            }
        }

        if (isInput) {
            int remaining = digits - number.length();
            for (int r = 0; r < remaining; r++) {
                for (int j = 0; j < 7; j++) {
                    symbolLines[j].push_back(CYAN + string(" _____ ") + RESET);
                }
            }
        }

        for (int i = 0; i < 7; i++) {
            string fullLine;
            for (const auto& part : symbolLines[i]) fullLine += part + " ";
            printInBorder(fullLine, true);
        }
    }

    // Menampilkan petunjuk panah di sebelah tebakan
    void displayArrowHint(const string& guess, const string& feedback) {
        if (guess == target) return;

        string arrowArt = (guess < target) ? arrowSymbols.at("UP") : arrowSymbols.at("DOWN");
        stringstream arrow_ss(arrowArt);
        vector<string> arrowLines;
        string arrowLine;
        while(getline(arrow_ss, arrowLine, '\n')) arrowLines.push_back(arrowLine);

        vector<vector<string>> guessLines(7);
        for (size_t i = 0; i < guess.size(); i++) {
            stringstream digit_ss(digitSymbols.at(guess[i]));
            vector<string> lines;
            string line;
            while(getline(digit_ss, line, '\n')) lines.push_back(line);

            string color = RESET;
            if (i < feedback.size()) {
                switch (feedback[i]) {
                    case 'G': color = GREEN; break;
                    case 'Y': color = YELLOW; break;
                    case 'X': color = RED; break;
                }
            }

            for (int j = 0; j < 7; j++) {
                guessLines[j].push_back(color + lines[j] + RESET);
            }
        }
        
        for (int j = 0; j < 7; j++) {
            guessLines[j].push_back("  " + arrowLines[j]);
        }
        
        for (int j = 0; j < 7; j++) {
            string fullLine;
            for (const auto& part : guessLines[j]) fullLine += part + " ";
            printInBorder(fullLine, true);
        }
    }

    // Menampilkan papan permainan utama
    void displayBoard() {
        clearScreen();
        printBorder(true);
        printInBorder("S Y M B O L I C   N U M B L E R", true);
        printInBorder("Tebak angka " + to_string(digits) + "-digit!", true);
        printBorder();
        printInBorder(string("Legenda: ") + GREEN + "Hijau" + RESET + "=Benar, " + YELLOW + "Kuning" + RESET + "=Posisi Salah, " + RED + "Merah" + RESET + "=Salah", true);
        printBorder();

        if (!guesses.empty()) {
            printInBorder("Tebakan sebelumnya:", false);
            printEmptyBorderLine();
            for (size_t i = 0; i < guesses.size(); i++) {
                printInBorder("Percobaan " + to_string(i + 1) + ":", false);
                if (guesses[i] != target) {
                    displayArrowHint(guesses[i], results[i]);
                } else {
                    displaySymbolicNumber(guesses[i], results[i]);
                }
                if (i < guesses.size() - 1) printEmptyBorderLine();
            }
            printBorder();
        }

        int remaining = maxAttempts - currentAttempt;
        printInBorder("Sisa percobaan: " + to_string(remaining), true);
        printBorder();
    }

    // LOGIKA PERMAINAN
    string evaluateGuess(const string& guess) {
        string result(guess.length(), ' ');
        vector<bool> targetUsed(target.length(), false);
        vector<bool> guessUsed(guess.length(), false);

        for (size_t i = 0; i < guess.length(); i++) {
            if (guess[i] == target[i]) {
                result[i] = 'G';
                targetUsed[i] = true;
                guessUsed[i] = true;
            }
        }
        for (size_t i = 0; i < guess.length(); i++) {
            if (!guessUsed[i]) {
                for (size_t j = 0; j < target.length(); j++) {
                    if (!targetUsed[j] && guess[i] == target[j]) {
                        result[i] = 'Y';
                        targetUsed[j] = true;
                        break;
                    }
                }
            }
        }
        for (size_t i = 0; i < result.length(); i++) {
            if (result[i] == ' ') result[i] = 'X';
        }
        return result;
    }
    
    // Mendapatkan input tebakan dari pemain
    string getGuessInput() {
        string input = "";
        while (true) {
            displayBoard();
            printInBorder("Tebakan saat ini:", false);
            displaySymbolicNumber(input, "", true);
            printBorder();
            printInBorder("Ketik 0-9, Backspace untuk hapus, Enter untuk kirim, Q untuk keluar", true);
            printBorder(false, true);

            char ch = getChar();
            if (ch == '\r' || ch == '\n') {
                if (input.length() == static_cast<size_t>(digits)) break;
                else {
                    clearScreen();
                    printBorder(true);
                    printInBorder(string(RED) + "Butuh tepat " + to_string(digits) + " digit!" + RESET, true);
                    printBorder(false, true);
                    pauseForInput();
                }
            } else if (ch == 8 || ch == 127) {
                if (!input.empty()) input.pop_back();
            } else if (isdigit(ch)) {
                if (input.length() < static_cast<size_t>(digits)) input += ch;
            } else if (tolower(ch) == 'q') {
                return "QUIT";
            }
        }
        return input;
    }
    
    // Menampilkan statistik akhir permainan
    void displayStats(bool won) {
        clearScreen();
        printBorder(true);
        if (won) {
            printInBorder("*** SELAMAT! ***", true);
            printInBorder("Anda menebaknya dalam " + to_string(currentAttempt) + " percobaan!", true);
        } else {
            printInBorder("*** PERMAINAN BERAKHIR! ***", true);
            printInBorder("Angkanya adalah:", true);
        }
        printBorder();
        displaySymbolicNumber(target);
        printBorder(false, true);
    }

    // Loop utama permainan
    void playGame() {
        while (currentAttempt < maxAttempts) {
            string guess = getGuessInput();
            if (guess == "QUIT") return;

            guesses.push_back(guess);
            results.push_back(evaluateGuess(guess));
            currentAttempt++;

            if (guess == target) {
                displayStats(true);
                int score = calculateScore();
                addScoreToLeaderboard(score);
                return;
            }
        }
        displayStats(false);
    }
    
    // Menampilkan instruksi
    void showInstructions() {
        clearScreen();
        printBorder(true);
        printInBorder("INSTRUKSI SYMBOLIC NUMBLER", true);
        printBorder();
        printInBorder("* Tebak angka rahasia " + to_string(digits) + "-digit.", false);
        printInBorder("* Anda memiliki " + to_string(maxAttempts) + " percobaan.", false);
        printInBorder(string("* ") + GREEN + "HIJAU" + RESET + ": Digit & posisi benar.", false);
        printInBorder(string("* ") + YELLOW + "KUNING" + RESET + ": Digit benar, posisi salah.", false);
        printInBorder(string("* ") + RED + "MERAH" + RESET + ": Digit salah.", false);
        printInBorder("* Panah akan menunjukkan angka rahasia lebih tinggi atau rendah.", false);
        printBorder(false, true);
        pauseForInput();
    }

    // FUNGSI PAPAN PERINGKAT
    void loadLeaderboard() {
        ifstream file(leaderboardFilename);
        if (!file.is_open()) return;
        leaderboard.clear();
        ScoreEntry entry;
        while (file >> entry.playerName >> entry.score >> entry.digits >> entry.attempts) {
            leaderboard.push_back(entry);
        }
        file.close();
    }

    void saveLeaderboard() {
        sort(leaderboard.begin(), leaderboard.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });
        ofstream file(leaderboardFilename);
        for (const auto& entry : leaderboard) {
            file << entry.playerName << " " << entry.score << " " << entry.digits << " " << entry.attempts << "\n";
        }
        file.close();
    }

    void addScoreToLeaderboard(int score) {
        cout << "\nSkor Anda: " << score << ". Masukkan nama (tanpa spasi): ";
        string name;
        cin >> name;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        leaderboard.push_back({name.substr(0, 15), score, digits, currentAttempt});
        sort(leaderboard.begin(), leaderboard.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });
        if (leaderboard.size() > 10) leaderboard.resize(10);
        
        saveLeaderboard();
        displayLeaderboard();
    }

    void displayLeaderboard() {
        clearScreen();
        printBorder(true);
        printInBorder("PAPAN PERINGKAT", true);
        printBorder();
        
        if (leaderboard.empty()) {
            printInBorder("Papan peringkat masih kosong. Jadilah yang pertama!", true);
        } else {
            cout << CYAN << "| " << RESET << left
                 << setw(5) << "No."
                 << setw(20) << "Nama"
                 << setw(15) << "Skor"
                 << setw(15) << "Digit"
                 << setw(15) << "Percobaan" << string(BORDER_WIDTH - 74, ' ') << CYAN << "|" << RESET << endl;
            printBorder();
            int rank = 1;
            for (const auto& entry : leaderboard) {
                cout << CYAN << "| " << RESET << left
                     << setw(5) << to_string(rank) + "."
                     << setw(20) << entry.playerName
                     << setw(15) << entry.score
                     << setw(15) << entry.digits
                     << setw(15) << entry.attempts << string(BORDER_WIDTH - 74, ' ') << CYAN << "|" << RESET << endl;
                rank++;
            }
        }
        
        printBorder(false, true);
        pauseForInput();
    }
};

// Fungsi untuk mendapatkan input integer yang aman
int getIntegerInput(const string& prompt, int min, int max) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if (cin.good() && value >= min && value <= max) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Input tidak valid. Masukkan angka antara " << min << " dan " << max << ".\n";
    }
}

/*
 * FUNGSI MAIN - TITIK MASUK PROGRAM
 */
int main() {
    SymbolicNumbler game; // Buat satu objek game di awal

    while (true) {
        SymbolicNumbler::clearScreen();
        cout << "\n";
        cout << "+============================================================+\n";
        cout << "|           Selamat Datang di SYMBOLIC NUMBLER!              |\n";
        cout << "+============================================================+\n\n";

        cout << "Pilih Menu:\n";
        cout << "1. Mudah (3 digit, 6 percobaan)\n";
        cout << "2. Sedang (4 digit, 6 percobaan)\n";
        cout << "3. Sulit (5 digit, 6 percobaan)\n";
        cout << "4. Ahli (6 digit, 8 percobaan)\n";
        cout << "5. Kustom\n";
        cout << "6. Papan Peringkat\n";
        cout << "7. Keluar\n";
        cout << "Pilihan (1-7): ";

        char choice = SymbolicNumbler::getChar();
        cout << choice << endl;

        if (choice == '7' || tolower(choice) == 'q') break;
        if (choice == '6') {
            game.displayLeaderboard();
            continue;
        }

        int digits, attempts;
        switch (choice) {
            case '1': digits = 3; attempts = 6; break;
            case '2': digits = 4; attempts = 6; break;
            case '3': digits = 5; attempts = 6; break;
            case '4': digits = 6; attempts = 8; break;
            case '5':
                digits = getIntegerInput("Masukkan jumlah digit (3-6): ", 3, 6);
                attempts = getIntegerInput("Masukkan jumlah percobaan (4-10): ", 4, 10);
                break;
            default:
                cout << "Pilihan tidak valid. Memulai tingkat kesulitan sedang...\n";
                digits = 4; attempts = 6;
                // Jeda agar pengguna bisa membaca pesan
                for(int i=0; i<100000000; ++i); // Delay sederhana
        }

        game.startNewGame(digits, attempts);
        game.showInstructions();
        game.playGame();

        cout << "\nMain lagi? (y/n): ";
        char playAgain = SymbolicNumbler::getChar();
        cout << playAgain << endl;
        if (tolower(playAgain) != 'y') break;
    }

    cout << "\nTerima kasih telah bermain SYMBOLIC NUMBLER!\n";
    return 0;
}
