#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_PLAYERS 100
#define MAX_GAMES 100
#define MAX_GAMES_PLAYED 100
#define MAX_GUESTS 100
#define MAX_PREFS 10
#define MAX_SESSION_PLAYERS 100   /* BUGFIX: was 10, but some games require up to 100 players */
#define DATA_FILE "game_data.dat"

// ============================================================
// Structures
// ============================================================
typedef struct {
    int games_played;
    int games_won;
    int total_score;
    int prizes;
} GameStats;

typedef struct {
    int player_id;
    char name[50];
    char registration_date[20];
    GameStats stats;
    int game_preferences[MAX_PREFS];
    int pref_count;
} Player;

typedef struct {
    int game_id;
    char name[50];
    int type; // 1 = "single", 2 = "multiple"
    int players_required;
    int prizesAwarded;
    int totalPlayTime;
} GameMaster;

typedef struct {
    int session_id;
    int game_id;
    int playerIds[MAX_SESSION_PLAYERS];
    int player_count;
    int scores[MAX_SESSION_PLAYERS];
    int prizes[MAX_SESSION_PLAYERS];
    int playTime[MAX_SESSION_PLAYERS];
    char start_date[20];
    char end_date[20];
    int is_completed;
    int players_quit[MAX_SESSION_PLAYERS];
} GamePlayed;

typedef struct {
    int guest_id;
    char name[50];
    char start_date[20];
    int active_days;
    int isBlocked;
} GuestUser;

// ============================================================
// Global data
// ============================================================
Player players[MAX_PLAYERS];
GameMaster games[MAX_GAMES];
GamePlayed games_played[MAX_GAMES_PLAYED];
GuestUser guests[MAX_GUESTS];

int player_count = 0;
int game_count = 0;
int total_sessions = 0;
int guest_count = 0;

int next_player_id = 101;
int next_game_id = 1;
int next_session_id = 1;
int next_guest_id = 1;

int data_dirty = 0; // tracks unsaved changes, used for friendly save prompts

// ============================================================
// Safe input helpers  (BUGFIX: replaces unchecked scanf calls
// that could overflow buffers or desync stdin on bad input)
// ============================================================
static void flush_stdin_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

int get_int(const char *prompt) {
    char buf[128];
    int value;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) {
            clearerr(stdin);
            continue;
        }
        // strip trailing newline if fgets captured full line, otherwise flush the rest
        if (!strchr(buf, '\n')) flush_stdin_line();
        if (sscanf(buf, " %d", &value) == 1) {
            return value;
        }
        printf("Invalid input. Please enter a whole number.\n");
    }
}

// Reads a string safely into dest (size-limited), trims newline
void get_string(const char *prompt, char *dest, size_t size) {
    char buf[256];
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) {
        clearerr(stdin);
        dest[0] = '\0';
        return;
    }
    if (!strchr(buf, '\n')) flush_stdin_line();
    buf[strcspn(buf, "\n")] = '\0';
    // trim leading spaces
    char *start = buf;
    while (*start == ' ') start++;
    strncpy(dest, start, size - 1);
    dest[size - 1] = '\0';
}

// Very small date sanity check: DD-MM-YYYY
int is_valid_date(const char *date) {
    int d, m, y;
    if (sscanf(date, "%d-%d-%d", &d, &m, &y) != 3) return 0;
    if (d < 1 || d > 31 || m < 1 || m > 12 || y < 1900 || y > 3000) return 0;
    return 1;
}

void get_valid_date(const char *prompt, char *dest, size_t size) {
    while (1) {
        get_string(prompt, dest, size);
        if (is_valid_date(dest)) return;
        printf("Invalid date format. Please use DD-MM-YYYY (e.g. 05-06-2024).\n");
    }
}

int days_difference(char startDate[], char endDate[]) {
    int startDay, startMonth, startYear;
    int endDay, endMonth, endYear;

    sscanf(startDate, "%d-%d-%d", &startDay, &startMonth, &startYear);
    sscanf(endDate, "%d-%d-%d", &endDay, &endMonth, &endYear);

    // (assuming 30 days per month, 365 days per year)
    int startTotalDays = startYear * 365 + startMonth * 30 + startDay;
    int endTotalDays = endYear * 365 + endMonth * 30 + endDay;

    return endTotalDays - startTotalDays;
}

// ============================================================
// Lookup helpers
// ============================================================
int find_player_by_id(int playerId) {
    for (int i = 0; i < player_count; i++) {
        if (players[i].player_id == playerId) return i;
    }
    return -1;
}

int find_game_by_id(int gameId) {
    for (int i = 0; i < game_count; i++) {
        if (games[i].game_id == gameId) return i;
    }
    return -1;
}

int find_guest_by_id(int guestId) {
    for (int i = 0; i < guest_count; i++) {
        if (guests[i].guest_id == guestId) return i;
    }
    return -1;
}

int find_session_by_id(int sessionId) {
    for (int i = 0; i < total_sessions; i++) {
        if (games_played[i].session_id == sessionId) return i;
    }
    return -1;
}

int count_player_active_games(int playerId) {
    int activeCount = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (!games_played[i].is_completed) {
            for (int j = 0; j < games_played[i].player_count; j++) {
                if (games_played[i].playerIds[j] == playerId && games_played[i].players_quit[j] == 0) {
                    activeCount++;
                    break;
                }
            }
        }
    }
    return activeCount;
}

int calculate_game_prizes(int gameId, int score) {
    int gameIndex = find_game_by_id(gameId);
    if (gameIndex == -1) return 0;
    if (score >= 1000) return 5;
    else if (score >= 500) return 3;
    else if (score >= 100) return 1;
    return 0;
}

// ============================================================
// File handling (NEW)
// ============================================================
void save_data() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) {
        printf("Error: Could not open '%s' for saving.\n", DATA_FILE);
        return;
    }

    fwrite(&player_count, sizeof(int), 1, fp);
    fwrite(&game_count, sizeof(int), 1, fp);
    fwrite(&total_sessions, sizeof(int), 1, fp);
    fwrite(&guest_count, sizeof(int), 1, fp);
    fwrite(&next_player_id, sizeof(int), 1, fp);
    fwrite(&next_game_id, sizeof(int), 1, fp);
    fwrite(&next_session_id, sizeof(int), 1, fp);
    fwrite(&next_guest_id, sizeof(int), 1, fp);

    fwrite(players, sizeof(Player), player_count, fp);
    fwrite(games, sizeof(GameMaster), game_count, fp);
    fwrite(games_played, sizeof(GamePlayed), total_sessions, fp);
    fwrite(guests, sizeof(GuestUser), guest_count, fp);

    fclose(fp);
    data_dirty = 0;
    printf("Data saved successfully to '%s'.\n", DATA_FILE);
}

// Returns 1 if a save file was found and loaded, 0 otherwise
int load_data() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return 0;

    int ok = 1;
    ok &= (fread(&player_count, sizeof(int), 1, fp) == 1);
    ok &= (fread(&game_count, sizeof(int), 1, fp) == 1);
    ok &= (fread(&total_sessions, sizeof(int), 1, fp) == 1);
    ok &= (fread(&guest_count, sizeof(int), 1, fp) == 1);
    ok &= (fread(&next_player_id, sizeof(int), 1, fp) == 1);
    ok &= (fread(&next_game_id, sizeof(int), 1, fp) == 1);
    ok &= (fread(&next_session_id, sizeof(int), 1, fp) == 1);
    ok &= (fread(&next_guest_id, sizeof(int), 1, fp) == 1);

    // Sanity-check counts before reading arrays, to avoid corrupt-file overflows
    if (!ok || player_count < 0 || player_count > MAX_PLAYERS ||
        game_count < 0 || game_count > MAX_GAMES ||
        total_sessions < 0 || total_sessions > MAX_GAMES_PLAYED ||
        guest_count < 0 || guest_count > MAX_GUESTS) {
        printf("Warning: Save file appears corrupted. Ignoring it.\n");
        fclose(fp);
        player_count = game_count = total_sessions = guest_count = 0;
        return 0;
    }

    if (player_count > 0) fread(players, sizeof(Player), player_count, fp);
    if (game_count > 0) fread(games, sizeof(GameMaster), game_count, fp);
    if (total_sessions > 0) fread(games_played, sizeof(GamePlayed), total_sessions, fp);
    if (guest_count > 0) fread(guests, sizeof(GuestUser), guest_count, fp);

    fclose(fp);
    printf("Data loaded successfully from '%s'.\n", DATA_FILE);
    return 1;
}

// ============================================================
// Player registration
// ============================================================
void register_new_player() {
    if (player_count >= MAX_PLAYERS) {
        printf("Player database full!\n");
        return;
    }
    Player new_player;
    new_player.player_id = next_player_id;

    get_string("Enter player name: ", new_player.name, sizeof(new_player.name));
    if (strlen(new_player.name) == 0) {
        printf("Player name cannot be empty. Registration cancelled.\n");
        return;
    }
    get_valid_date("Enter registration date (DD-MM-YYYY): ", new_player.registration_date, sizeof(new_player.registration_date));

    new_player.stats.games_played = 0;
    new_player.stats.games_won = 0;
    new_player.stats.total_score = 0;
    new_player.stats.prizes = 0;
    new_player.pref_count = 0;

    if (game_count > 0) {
        GameMaster tempGames[MAX_GAMES];
        for (int i = 0; i < game_count; i++) tempGames[i] = games[i];

        for (int i = 0; i < game_count - 1; i++) {
            for (int j = 0; j < game_count - i - 1; j++) {
                if (tempGames[j].totalPlayTime < tempGames[j + 1].totalPlayTime) {
                    GameMaster temp = tempGames[j];
                    tempGames[j] = tempGames[j + 1];
                    tempGames[j + 1] = temp;
                }
            }
        }

        int gamesToAssign = (game_count < 3) ? game_count : 3;
        for (int i = 0; i < gamesToAssign; i++) {
            if (new_player.pref_count < MAX_PREFS) {
                new_player.game_preferences[new_player.pref_count++] = tempGames[i].game_id;
            }
        }

        if (new_player.pref_count > 0) {
            printf("Popular game preferences assigned (based on playtime):\n");
            for (int i = 0; i < new_player.pref_count; i++) {
                int gindex = find_game_by_id(new_player.game_preferences[i]);
                if (gindex != -1) {
                    printf("  - %s (ID: %d) - Total Play Time: %d days\n",
                           games[gindex].name, games[gindex].game_id, games[gindex].totalPlayTime);
                }
            }
        }
    } else {
        printf("No games available in the system! Add game preferences later.\n");
    }

    players[player_count++] = new_player;
    next_player_id++;
    data_dirty = 1;
    printf("\nPlayer registered successfully with ID: %d\n", new_player.player_id);
}

// ============================================================
// Display games
// ============================================================
void display_games() {
    if (game_count == 0) {
        printf("\nNo games available in the system.\n");
        return;
    }
    printf("\n--- Available Games ---\n");
    printf("%-6s %-20s %-10s %-16s\n", "ID", "Name", "Type", "Players Req.");
    printf("%-6s %-20s %-10s %-16s\n", "------", "--------------------", "----------", "----------------");
    for (int i = 0; i < game_count; i++) {
        printf("%-6d %-20s %-10s %-16d\n",
               games[i].game_id,
               games[i].name,
               games[i].type == 1 ? "Single" : "Multiple",
               games[i].players_required);
    }
}

// ============================================================
// Game preferences
// ============================================================
void add_game_preferences(int playerId) {
    int playerIndex = find_player_by_id(playerId);
    if (playerIndex == -1) {
        printf("Player not found!\n");
        return;
    }

    if (players[playerIndex].pref_count >= MAX_PREFS) {
        printf("Maximum game preferences reached for this player!\n");
        return;
    }

    display_games();
    if (game_count == 0) return;

    int gameId = get_int("Enter Game ID to add to preferences (0 to stop): ");
    if (gameId == 0) return;

    if (find_game_by_id(gameId) == -1) {
        printf("Invalid Game ID!\n");
        return;
    }

    for (int i = 0; i < players[playerIndex].pref_count; i++) {
        if (players[playerIndex].game_preferences[i] == gameId) {
            printf("Game already in preferences!\n");
            return;
        }
    }

    players[playerIndex].game_preferences[players[playerIndex].pref_count++] = gameId;
    data_dirty = 1;
    printf("Game added to preferences successfully!\n");
}

// ============================================================
// Start game session
// ============================================================
void start_game_session() {
    if (total_sessions >= MAX_GAMES_PLAYED) {
        printf("Maximum game sessions reached!\n");
        return;
    }
    if (game_count == 0) {
        printf("No games available. Cannot start a session.\n");
        return;
    }

    display_games();
    int game_id = get_int("\nEnter Game ID: ");
    int gameIndex = find_game_by_id(game_id);
    if (gameIndex == -1) {
        printf("Game not found!\n");
        return;
    }

    // BUGFIX: guard against a game whose players_required exceeds our array capacity
    int requiredPlayers = games[gameIndex].players_required;
    if (requiredPlayers > MAX_SESSION_PLAYERS) requiredPlayers = MAX_SESSION_PLAYERS;

    GamePlayed new_session;
    new_session.session_id = next_session_id;
    new_session.game_id = game_id;

    get_valid_date("Enter session start date (DD-MM-YYYY): ", new_session.start_date, sizeof(new_session.start_date));
    strcpy(new_session.end_date, "");
    new_session.player_count = 0;
    new_session.is_completed = 0;

    for (int i = 0; i < MAX_SESSION_PLAYERS; i++) new_session.players_quit[i] = 0;

    printf("\nAdding players to the session (need %d player(s)):\n", requiredPlayers);
    printf("Enter Player ID (0 for guest, -1 to stop):\n");

    while (new_session.player_count < requiredPlayers) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Player %d: ", new_session.player_count + 1);
        int playerId = get_int(prompt);

        if (playerId == -1) break;

        if (playerId == 0) {
            if (guest_count >= MAX_GUESTS) {
                printf("Guest user database full!\n");
                continue;
            }
            GuestUser new_guest;
            new_guest.guest_id = next_guest_id;
            get_string("Enter guest name: ", new_guest.name, sizeof(new_guest.name));
            if (strlen(new_guest.name) == 0) {
                printf("Guest name cannot be empty. Skipping.\n");
                continue;
            }
            strcpy(new_guest.start_date, new_session.start_date);
            new_guest.active_days = 0;
            new_guest.isBlocked = 0;

            guests[guest_count++] = new_guest;
            next_guest_id++;
            playerId = -new_guest.guest_id;
            data_dirty = 1;
            printf("Guest '%s' created with ID: %d\n", new_guest.name, new_guest.guest_id);
        } else {
            int playerIndex = find_player_by_id(playerId);
            if (playerIndex == -1) {
                printf("Player not found! Please check the Player ID.\n");
                continue;
            }
        }

        // BUGFIX: prevent adding the same player/guest twice to one session
        int alreadyInSession = 0;
        for (int i = 0; i < new_session.player_count; i++) {
            if (new_session.playerIds[i] == playerId) { alreadyInSession = 1; break; }
        }
        if (alreadyInSession) {
            printf("This player/guest is already in the session.\n");
            continue;
        }

        if (playerId < 0) {
            int guestIndex = find_guest_by_id(-playerId);
            if (guestIndex != -1 && guests[guestIndex].isBlocked) {
                printf("This guest is currently blocked.\n");
                continue;
            }
        }

        new_session.playerIds[new_session.player_count] = playerId;
        new_session.scores[new_session.player_count] = 0;
        new_session.prizes[new_session.player_count] = 0;
        new_session.playTime[new_session.player_count] = 0;
        new_session.players_quit[new_session.player_count] = 0;
        new_session.player_count++;

        if (playerId > 0) {
            int playerIndex = find_player_by_id(playerId);
            if (playerIndex != -1) players[playerIndex].stats.games_played++;
        }

        if (new_session.player_count >= requiredPlayers) {
            printf("Required number of players reached for the session.\n");
            break;
        }
    }

    if (new_session.player_count == 0) {
        printf("No players added to the session. Session not created.\n");
        return;
    }

    games_played[total_sessions++] = new_session;
    next_session_id++;
    data_dirty = 1;

    printf("\nGame session started successfully with Session ID: %d\n", new_session.session_id);
    printf("Game: %s (ID: %d)\n", games[gameIndex].name, new_session.game_id);
    printf("--------------------------------------------\n");
    for (int i = 0; i < new_session.player_count; i++) {
        int pid = new_session.playerIds[i];
        if (pid > 0) {
            int pIndex = find_player_by_id(pid);
            if (pIndex != -1) printf("Player: %s (ID: %d)\n", players[pIndex].name, pid);
        } else {
            int gIndex = find_guest_by_id(-pid);
            if (gIndex != -1) printf("Guest: %s (ID: %d)\n", guests[gIndex].name, -pid);
        }
    }
    printf("--------------------------------------------\n");
    printf("Total Players: %d\n", new_session.player_count);
}

// ============================================================
// Quit game session
// ============================================================
void quit_game_session() {
    printf("\n----- QUIT GAME SESSION -----\n");
    printf("%-11s %-8s %-16s %-8s %s\n", "Session ID", "Game ID", "Game Name", "Players", "Start Date");
    printf("%-11s %-8s %-16s %-8s %s\n", "-----------", "-------", "----------------", "-------", "----------");

    int activeSessionsFound = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (!games_played[i].is_completed) {
            int gameIndex = find_game_by_id(games_played[i].game_id);
            if (gameIndex != -1) {
                printf("%-11d %-8d %-16s %-8d %s\n",
                       games_played[i].session_id, games_played[i].game_id,
                       games[gameIndex].name, games_played[i].player_count,
                       games_played[i].start_date);
                activeSessionsFound = 1;
            }
        }
    }

    if (!activeSessionsFound) {
        printf("No active game sessions found.\n");
        return;
    }

    int sessionId = get_int("\nEnter Session ID to quit: ");
    int sessionIndex = find_session_by_id(sessionId);

    if (sessionIndex == -1) {
        printf("Session not found!\n");
        return;
    }
    if (games_played[sessionIndex].is_completed) {
        printf("This session is already completed!\n");
        return;
    }

    GamePlayed *session = &games_played[sessionIndex];
    int gameIndex = find_game_by_id(session->game_id);

    printf("\nPlayers in Session %d (%s):\n", sessionId, gameIndex != -1 ? games[gameIndex].name : "Unknown");
    printf("%-6s %-11s %-16s %s\n", "Index", "Player ID", "Name", "Quit Status");
    printf("------------------------------------------------\n");
    for (int i = 0; i < session->player_count; i++) {
        int playerId = session->playerIds[i];
        const char *status = session->players_quit[i] ? "Quit" : "Active";
        if (playerId > 0) {
            int playerIndex = find_player_by_id(playerId);
            if (playerIndex != -1) printf("%-6d %-11d %-16s %s\n", i + 1, playerId, players[playerIndex].name, status);
        } else {
            int guestIndex = find_guest_by_id(-playerId);
            if (guestIndex != -1) printf("%-6d %-11d %-16s %s\n", i + 1, playerId, guests[guestIndex].name, status);
        }
    }

    char prompt[64];
    snprintf(prompt, sizeof(prompt), "\nEnter Player Index to quit (1 to %d): ", session->player_count);
    int playerIndexToQuit = get_int(prompt);

    if (playerIndexToQuit < 1 || playerIndexToQuit > session->player_count) {
        printf("Invalid player index!\n");
        return;
    }
    if (session->players_quit[playerIndexToQuit - 1] == 1) {
        printf("This player has already quit the session!\n");
        return;
    }

    int quittingPlayerId = session->playerIds[playerIndexToQuit - 1];
    if (quittingPlayerId < 0) {
        int guestIndex = find_guest_by_id(-quittingPlayerId);
        if (guestIndex != -1) printf("Guest '%s' (ID: %d) is about to quit the session.\n", guests[guestIndex].name, -quittingPlayerId);
    } else {
        int playerIndex = find_player_by_id(quittingPlayerId);
        if (playerIndex != -1) printf("Player '%s' (ID: %d) is about to quit the session.\n", players[playerIndex].name, quittingPlayerId);
    }

    int confirm = get_int("Are you sure you want to quit? (1 = Yes, 0 = No): ");
    if (confirm != 1) {
        printf("Quit action cancelled.\n");
        return;
    }

    session->players_quit[playerIndexToQuit - 1] = 1;
    printf("Player has been marked as quit in the session.\n");

    if (quittingPlayerId > 0) {
        int playerIndex = find_player_by_id(quittingPlayerId);
        if (playerIndex != -1) {
            if (players[playerIndex].stats.games_played > 0) players[playerIndex].stats.games_played--;
            printf("Game count updated for player '%s'.\n", players[playerIndex].name);
        }
    }

    int allQuit = 1;
    for (int i = 0; i < session->player_count; i++) {
        if (session->players_quit[i] == 0) { allQuit = 0; break; }
    }

    if (allQuit) {
        printf("All players have quit the session.\n");
        session->is_completed = 1;
        printf("Session marked as completed.\n");
        get_valid_date("Enter session end date (DD-MM-YYYY): ", session->end_date, sizeof(session->end_date));
    }

    data_dirty = 1;
    printf("Quit operation completed.\n");
}

// ============================================================
// End game session
// ============================================================
void end_game_session() {
    printf("\n=== END GAME SESSION ===\n");
    printf("%-11s %-8s %-16s %-8s %s\n", "Session ID", "Game ID", "Game Name", "Players", "Start Date");
    printf("%-11s %-8s %-16s %-8s %s\n", "-----------", "-------", "----------------", "-------", "----------");

    int activeSessionsFound = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (!games_played[i].is_completed) {
            int gameIndex = find_game_by_id(games_played[i].game_id);
            printf("%-11d %-8d %-16s %-8d %s\n",
                   games_played[i].session_id, games_played[i].game_id,
                   gameIndex != -1 ? games[gameIndex].name : "Unknown",
                   games_played[i].player_count, games_played[i].start_date);
            activeSessionsFound = 1;
        }
    }

    if (!activeSessionsFound) {
        printf("No active game sessions found.\n");
        return;
    }

    int sessionId = get_int("\nEnter Session ID to end: ");
    int sessionIndex = find_session_by_id(sessionId);

    if (sessionIndex == -1) { printf("Session not found!\n"); return; }
    if (games_played[sessionIndex].is_completed) { printf("This session is already completed!\n"); return; }

    GamePlayed *session = &games_played[sessionIndex];
    char endDate[20];
    int daysActive;

    while (1) {
        get_valid_date("Enter session end date (DD-MM-YYYY): ", endDate, sizeof(endDate));
        daysActive = days_difference(session->start_date, endDate);
        if (daysActive < 0) {
            printf("End date cannot be before start date. Please re-enter.\n");
            continue;
        }
        break;
    }

    strcpy(session->end_date, endDate);
    if (daysActive == 0) daysActive = 1;

    printf("Enter scores for each player:\n");
    int totalPrizesAwarded = 0;
    int bestScore = -1;
    int winningPlayerId = -1;
    int sessionTotalPlayTime = daysActive;

    for (int i = 0; i < session->player_count; i++) {
        int playerId = session->playerIds[i];

        if (session->players_quit[i] == 1) {
            if (playerId > 0) {
                int playerIndex = find_player_by_id(playerId);
                if (playerIndex != -1) printf("Player %s has quit the session. Skipping score entry.\n", players[playerIndex].name);
            } else {
                int guestIndex = find_guest_by_id(-playerId);
                if (guestIndex != -1) printf("Guest %s has quit the session. Skipping score entry.\n", guests[guestIndex].name);
            }
            session->scores[i] = 0;
            session->prizes[i] = 0;
            session->playTime[i] = 0;
            continue;
        }

        char prompt[80];
        if (playerId > 0) {
            int playerIndex = find_player_by_id(playerId);
            snprintf(prompt, sizeof(prompt), "Enter score for Player %s (ID: %d): ",
                     playerIndex != -1 ? players[playerIndex].name : "?", playerId);
        } else {
            int guestIndex = find_guest_by_id(-playerId);
            snprintf(prompt, sizeof(prompt), "Enter score for Guest %s (ID: %d): ",
                     guestIndex != -1 ? guests[guestIndex].name : "?", -playerId);
        }
        session->scores[i] = get_int(prompt);
        if (session->scores[i] < 0) session->scores[i] = 0; // BUGFIX: reject negative scores

        if (playerId < 0 && daysActive > 15) {
            printf("  Guest players cannot play more than 15 days. Limiting playtime to 15 days.\n");
            int guestIndex = find_guest_by_id(-playerId);
            session->playTime[i] = 15;
            if (guestIndex != -1) {
                guests[guestIndex].active_days = 15;
                guests[guestIndex].isBlocked = 1;
                printf("  Guest %s (ID: %d) has been blocked due to excessive playtime.\n", guests[guestIndex].name, -playerId);
            }
        } else {
            session->playTime[i] = daysActive;
        }

        session->prizes[i] = calculate_game_prizes(session->game_id, session->scores[i]);
        totalPrizesAwarded += session->prizes[i];

        if (session->scores[i] > bestScore) {
            bestScore = session->scores[i];
            winningPlayerId = playerId;
        }

        printf("  Active days: %d, Prizes earned: %d\n", session->playTime[i], session->prizes[i]);

        if (playerId > 0) {
            int playerIndex = find_player_by_id(playerId);
            if (playerIndex != -1) {
                players[playerIndex].stats.total_score += session->scores[i];
                players[playerIndex].stats.prizes += session->prizes[i];
            }
        } else {
            int guestIndex = find_guest_by_id(-playerId);
            if (guestIndex != -1 && !guests[guestIndex].isBlocked) {
                guests[guestIndex].active_days += daysActive;
                if (guests[guestIndex].active_days >= 20) {
                    guests[guestIndex].isBlocked = 1;
                    printf("  Guest %s (ID: %d) has been blocked due to excessive total playtime.\n", guests[guestIndex].name, -playerId);
                }
            }
        }
    }

    if (winningPlayerId > 0) {
        int winnerIndex = find_player_by_id(winningPlayerId);
        if (winnerIndex != -1) {
            players[winnerIndex].stats.games_won++;
            printf("Player %s (ID: %d) is the winner of this session with a score of %d!\n",
                   players[winnerIndex].name, winningPlayerId, bestScore);
        }
    }

    int gameIndex = find_game_by_id(session->game_id);
    if (gameIndex != -1) {
        games[gameIndex].prizesAwarded += totalPrizesAwarded;
        games[gameIndex].totalPlayTime += sessionTotalPlayTime;
    }

    session->is_completed = 1;
    data_dirty = 1;

    printf("\nGame session %d ended successfully.\n", sessionId);
    printf("Total Prizes Awarded in this session: %d\n", totalPrizesAwarded);
    printf("Session duration: %d days\n", daysActive);
}

// ============================================================
// Search: games played by others
// ============================================================
void search_games_played_by_others() {
    int playerId = get_int("Enter Player ID to search for: ");
    int gameId = get_int("Enter Game ID to search for: ");

    int playerIndex = find_player_by_id(playerId);
    if (playerIndex == -1) { printf("Player with ID %d not found!\n", playerId); return; }

    int gameIndex = find_game_by_id(gameId);
    if (gameIndex == -1) { printf("Game with ID %d not found!\n", gameId); return; }

    char gameName[50];
    strcpy(gameName, games[gameIndex].name);
    char playerName[50];
    strcpy(playerName, players[playerIndex].name);

    printf("\n=== PLAYERS WHO PLAYED %s (Game ID %d) ===\n", gameName, gameId);
    printf("Player %s (ID: %d) wants to see who else played this game and what other games they play\n\n", playerName, playerId);

    int foundPlayers[MAX_PLAYERS];
    int foundGuests[MAX_GUESTS];
    int playerCnt = 0;
    int guestCnt = 0;

    printf("Players who played %s:\n", gameName);
    printf("%-11s %-9s %-9s %-16s %s\n", "Session ID", "Player ID", "Type", "Name", "Score");
    printf("%-11s %-9s %-9s %-16s %s\n", "-----------", "---------", "---------", "----------------", "-----");

    int foundGamePlayers = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (games_played[i].game_id == gameId && games_played[i].is_completed) {
            for (int j = 0; j < games_played[i].player_count; j++) {
                int pid = games_played[i].playerIds[j];
                if (pid == playerId) continue;

                if (pid > 0) {
                    int pIndex = find_player_by_id(pid);
                    if (pIndex != -1) {
                        printf("%-11d %-9d %-9s %-16s %d\n",
                               games_played[i].session_id, pid, "Player", players[pIndex].name, games_played[i].scores[j]);
                        if (playerCnt < MAX_PLAYERS) foundPlayers[playerCnt++] = pid;
                        foundGamePlayers = 1;
                    }
                } else {
                    int gIndex = find_guest_by_id(-pid);
                    if (gIndex != -1) {
                        printf("%-11d %-9d %-9s %-16s %d\n",
                               games_played[i].session_id, pid, "Guest", guests[gIndex].name, games_played[i].scores[j]);
                        if (guestCnt < MAX_GUESTS) foundGuests[guestCnt++] = -pid;
                        foundGamePlayers = 1;
                    }
                }
            }
        }
    }

    if (!foundGamePlayers) {
        printf("No other players found who played '%s' (Game ID: %d).\n", gameName, gameId);
        return;
    }

    printf("\n=== OTHER GAMES PLAYED BY THESE PLAYERS ===\n");
    int foundOtherGames = 0;

    for (int i = 0; i < playerCnt; i++) {
        int currentPlayerId = foundPlayers[i];
        int currentPlayerIndex = find_player_by_id(currentPlayerId);
        if (currentPlayerIndex == -1) continue;

        printf("\nPlayer: %s (ID: %d) also plays:\n", players[currentPlayerIndex].name, currentPlayerId);

        if (players[currentPlayerIndex].pref_count > 0) {
            printf("  Game Preferences: ");
            for (int j = 0; j < players[currentPlayerIndex].pref_count; j++) {
                int prefGameId = players[currentPlayerIndex].game_preferences[j];
                int prefGameIndex = find_game_by_id(prefGameId);
                if (prefGameIndex != -1 && prefGameId != gameId) {
                    printf("%s (ID: %d) ", games[prefGameIndex].name, prefGameId);
                    foundOtherGames = 1;
                }
            }
            printf("\n");
        }

        printf("  Games Played: ");
        int sessionGamesFound = 0;
        for (int j = 0; j < total_sessions; j++) {
            if (!games_played[j].is_completed) continue;
            for (int k = 0; k < games_played[j].player_count; k++) {
                if (games_played[j].playerIds[k] == currentPlayerId && games_played[j].game_id != gameId) {
                    int sgi = find_game_by_id(games_played[j].game_id);
                    if (sgi != -1) {
                        printf("%s (Score: %d) ", games[sgi].name, games_played[j].scores[k]);
                        sessionGamesFound = 1;
                        foundOtherGames = 1;
                    }
                }
            }
        }
        if (!sessionGamesFound) printf("No other games played");
        printf("\n");
    }

    for (int i = 0; i < guestCnt; i++) {
        int currentGuestId = foundGuests[i];
        int currentGuestIndex = find_guest_by_id(currentGuestId);
        if (currentGuestIndex == -1) continue;

        printf("\nGuest: %s (ID: %d) also played:\n", guests[currentGuestIndex].name, currentGuestId);
        int guestGamesFound = 0;
        printf("  Games Played: ");
        for (int j = 0; j < total_sessions; j++) {
            if (!games_played[j].is_completed) continue;
            for (int k = 0; k < games_played[j].player_count; k++) {
                if (games_played[j].playerIds[k] == -currentGuestId && games_played[j].game_id != gameId) {
                    int sgi = find_game_by_id(games_played[j].game_id);
                    if (sgi != -1) {
                        printf("%s (Score: %d) ", games[sgi].name, games_played[j].scores[k]);
                        guestGamesFound = 1;
                        foundOtherGames = 1;
                    }
                }
            }
        }
        if (!guestGamesFound) printf("No other games played");
        printf("\n");
    }

    if (!foundOtherGames) printf("\nNo other games found for these players.\n");
}

// ============================================================
// Game-wise inactive users
// BUGFIX: original loop used game_count as the bound while
// indexing games_played[] (a different array) — this skipped or
// misread sessions. Now correctly loops over total_sessions.
// ============================================================
void display_gamewise_inactive_users() {
    printf("\nGame-wise Inactive Users (players still in an unfinished session, but not yet quit):\n");
    printf("%-8s %-16s %-11s %s\n", "Game ID", "Game Name", "Player ID", "Player Name");
    printf("%-8s %-16s %-11s %s\n", "-------", "----------------", "-----------", "-----------");

    int found = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (games_played[i].is_completed) continue;
        int gIndex = find_game_by_id(games_played[i].game_id);
        if (gIndex == -1) continue;

        for (int j = 0; j < games_played[i].player_count; j++) {
            if (games_played[i].players_quit[j] != 0) continue;
            int pid = games_played[i].playerIds[j];
            if (pid <= 0) continue; // registered players only
            int pIndex = find_player_by_id(pid);
            if (pIndex == -1) continue;
            printf("%-8d %-16s %-11d %s\n", games[gIndex].game_id, games[gIndex].name, pid, players[pIndex].name);
            found = 1;
        }
    }

    if (!found) printf("No inactive users found.\n");
}

// ============================================================
// Active users
// ============================================================
void display_active_users() {
    int k = get_int("Enter minimum number of active games (K): ");

    printf("\nPlayers with more than %d active games:\n", k);
    printf("%-11s %-16s %s\n", "Player ID", "Player Name", "Active Games");
    printf("%-11s %-16s %s\n", "-----------", "----------------", "------------");

    int found = 0;
    for (int i = 0; i < player_count; i++) {
        int activeGames = count_player_active_games(players[i].player_id);
        if (activeGames > k) {
            printf("%-11d %-16s %d\n", players[i].player_id, players[i].name, activeGames);
            found = 1;
        }
    }

    if (!found) printf("No players found with more than %d active games.\n", k);
}

// ============================================================
// Sorting displays
// ============================================================
void display_games_with_most_prizes() {
    if (game_count == 0) { printf("No games available.\n"); return; }
    GameMaster tempGames[MAX_GAMES];
    for (int i = 0; i < game_count; i++) tempGames[i] = games[i];

    for (int i = 0; i < game_count - 1; i++) {
        for (int j = 0; j < game_count - i - 1; j++) {
            if (tempGames[j].prizesAwarded < tempGames[j + 1].prizesAwarded) {
                GameMaster temp = tempGames[j];
                tempGames[j] = tempGames[j + 1];
                tempGames[j + 1] = temp;
            }
        }
    }

    printf("\nGames with Most Prizes Awarded:\n");
    printf("%-8s %-16s %s\n", "Game ID", "Game Name", "Prizes Awarded");
    printf("%-8s %-16s %s\n", "-------", "----------------", "--------------");
    for (int i = 0; i < game_count; i++) {
        printf("%-8d %-16s %d\n", tempGames[i].game_id, tempGames[i].name, tempGames[i].prizesAwarded);
    }
}

void display_top5_longest_played_games() {
    if (game_count == 0) { printf("No games available.\n"); return; }
    GameMaster tempGames[MAX_GAMES];
    for (int i = 0; i < game_count; i++) tempGames[i] = games[i];

    for (int i = 0; i < game_count - 1; i++) {
        for (int j = 0; j < game_count - i - 1; j++) {
            if (tempGames[j].totalPlayTime < tempGames[j + 1].totalPlayTime) {
                GameMaster temp = tempGames[j];
                tempGames[j] = tempGames[j + 1];
                tempGames[j + 1] = temp;
            }
        }
    }

    printf("\nTop 5 Longest Played Games:\n");
    printf("%-8s %-16s %s\n", "Game ID", "Game Name", "Total Play Time (days)");
    printf("%-8s %-16s %s\n", "-------", "----------------", "----------------------");

    int topCount = game_count < 5 ? game_count : 5;
    for (int i = 0; i < topCount; i++) {
        printf("%-8d %-16s %d\n", tempGames[i].game_id, tempGames[i].name, tempGames[i].totalPlayTime);
    }
}

void display_players_of_game() {
    if (game_count == 0) { printf("No games available.\n"); return; }
    display_games();
    int gameId = get_int("\nEnter Game ID: ");
    if (find_game_by_id(gameId) == -1) { printf("Game not found!\n"); return; }

    int playingPlayers[MAX_PLAYERS];
    int playerScores[MAX_PLAYERS];
    int playingCount = 0;

    for (int i = 0; i < total_sessions; i++) {
        if (games_played[i].game_id == gameId && !games_played[i].is_completed) {
            for (int j = 0; j < games_played[i].player_count; j++) {
                if (games_played[i].players_quit[j] == 0 && games_played[i].playerIds[j] > 0 && playingCount < MAX_PLAYERS) {
                    playingPlayers[playingCount] = games_played[i].playerIds[j];
                    playerScores[playingCount] = games_played[i].scores[j];
                    playingCount++;
                }
            }
        }
    }

    if (playingCount == 0) {
        printf("No active players found for Game ID %d.\n", gameId);
        return;
    }

    for (int i = 0; i < playingCount - 1; i++) {
        for (int j = 0; j < playingCount - i - 1; j++) {
            if (playerScores[j] < playerScores[j + 1]) {
                int tempScore = playerScores[j];
                playerScores[j] = playerScores[j + 1];
                playerScores[j + 1] = tempScore;
                int tempId = playingPlayers[j];
                playingPlayers[j] = playingPlayers[j + 1];
                playingPlayers[j + 1] = tempId;
            }
        }
    }

    printf("\nPlayers currently playing Game ID %d:\n", gameId);
    printf("%-11s %-16s %s\n", "Player ID", "Player Name", "Score");
    printf("%-11s %-16s %s\n", "-----------", "----------------", "-----");
    for (int i = 0; i < playingCount; i++) {
        int playerIndex = find_player_by_id(playingPlayers[i]);
        if (playerIndex != -1) printf("%-11d %-16s %d\n", playingPlayers[i], players[playerIndex].name, playerScores[i]);
    }
}

// ============================================================
// Delete operations
// ============================================================
void remove_inactive_guests() {
    printf("\nRemoving guests active for more than 20 days...\n");
    int removedCount = 0;

    for (int i = guest_count - 1; i >= 0; i--) {
        if (guests[i].active_days > 20) {
            printf("Removing Guest %s (ID: %d) - Active Days: %d\n", guests[i].name, guests[i].guest_id, guests[i].active_days);

            for (int j = 0; j < total_sessions; j++) {
                for (int k = 0; k < games_played[j].player_count; k++) {
                    if (games_played[j].playerIds[k] == -guests[i].guest_id) {
                        games_played[j].players_quit[k] = 1;
                        printf("  - Marked as quit in Session ID %d\n", games_played[j].session_id);
                    }
                }
            }

            for (int j = i; j < guest_count - 1; j++) guests[j] = guests[j + 1];
            guest_count--;
            removedCount++;
        }
    }

    if (removedCount > 0) data_dirty = 1;
    printf("Total guests removed: %d\n", removedCount);
}

// ============================================================
// Sample data (used only when no save file exists)
// ============================================================
void initialize_sample_data() {
    const char *gnames[10] = {"Chess","Free Fire","Among Us","Temple Run","Call of Duty",
                               "Subway Surfers","Ludo","Sudoku","BGMI","Candy Crush"};
    int gtypes[10]    = {1,2,2,1,2,1,2,1,2,1};
    int greqs[10]     = {1,50,4,1,10,1,4,1,100,1};
    int gprizes[10]   = {25,45,30,20,35,28,50,22,40,18};
    int gplaytime[10] = {120,85,200,75,60,95,180,70,150,45};

    for (int i = 0; i < 10; i++) {
        games[i].game_id = next_game_id++;
        strcpy(games[i].name, gnames[i]);
        games[i].type = gtypes[i];
        games[i].players_required = greqs[i];
        games[i].prizesAwarded = gprizes[i];
        games[i].totalPlayTime = gplaytime[i];
    }
    game_count = 10;

    struct { const char *name; const char *date; int played, won, score, prizes; int prefs[5]; int prefCount; } pdata[5] = {
        {"Raj","15-01-2024", 8,5,2500,15, {1,3,6,8,0}, 4},
        {"Ram","20-02-2024", 12,8,3200,20, {2,4,7,9,5}, 5},
        {"Dev","10-03-2024", 6,3,1800,8,  {5,8,1,4,0}, 4},
        {"Sri","05-04-2024", 4,2,900,4,   {3,6,9,0,0}, 3},
        {"Sai","12-04-2024", 10,6,2800,18,{2,7,1,8,4}, 5},
    };

    for (int i = 0; i < 5; i++) {
        players[i].player_id = next_player_id++;
        strcpy(players[i].name, pdata[i].name);
        strcpy(players[i].registration_date, pdata[i].date);
        players[i].stats.games_played = pdata[i].played;
        players[i].stats.games_won = pdata[i].won;
        players[i].stats.total_score = pdata[i].score;
        players[i].stats.prizes = pdata[i].prizes;
        players[i].pref_count = pdata[i].prefCount;
        for (int j = 0; j < pdata[i].prefCount; j++) players[i].game_preferences[j] = pdata[i].prefs[j];
    }
    player_count = 5;

    struct { const char *name; const char *date; int active; int blocked; } gdata[3] = {
        {"Siva","01-04-2024", 12, 0},
        {"Dil","05-04-2024", 25, 1},
        {"Nag","10-04-2024", 8, 0},
    };
    for (int i = 0; i < 3; i++) {
        guests[i].guest_id = next_guest_id++;
        strcpy(guests[i].name, gdata[i].name);
        strcpy(guests[i].start_date, gdata[i].date);
        guests[i].active_days = gdata[i].active;
        guests[i].isBlocked = gdata[i].blocked;
    }
    guest_count = 3;

    // Session 0
    games_played[0] = (GamePlayed){0};
    games_played[0].session_id = next_session_id++;
    games_played[0].game_id = 1;
    games_played[0].playerIds[0] = 101; games_played[0].playerIds[1] = 102;
    games_played[0].player_count = 2;
    games_played[0].scores[0] = 850; games_played[0].scores[1] = 720;
    games_played[0].prizes[0] = 3; games_played[0].prizes[1] = 1;
    games_played[0].playTime[0] = 5; games_played[0].playTime[1] = 5;
    strcpy(games_played[0].start_date, "10-04-2024");
    strcpy(games_played[0].end_date, "15-04-2024");
    games_played[0].is_completed = 1;

    // Session 1
    games_played[1] = (GamePlayed){0};
    games_played[1].session_id = next_session_id++;
    games_played[1].game_id = 3;
    games_played[1].playerIds[0] = 101; games_played[1].playerIds[1] = 103; games_played[1].playerIds[2] = -1;
    games_played[1].player_count = 3;
    games_played[1].scores[0] = 450; games_played[1].scores[1] = 380; games_played[1].scores[2] = 290;
    games_played[1].prizes[0] = 1;
    games_played[1].playTime[0] = 8; games_played[1].playTime[1] = 8; games_played[1].playTime[2] = 8;
    strcpy(games_played[1].start_date, "18-04-2024");
    strcpy(games_played[1].end_date, "26-04-2024");
    games_played[1].is_completed = 1;

    // Session 2 (active)
    games_played[2] = (GamePlayed){0};
    games_played[2].session_id = next_session_id++;
    games_played[2].game_id = 2;
    games_played[2].playerIds[0] = 102; games_played[2].playerIds[1] = 103; games_played[2].playerIds[2] = -2;
    games_played[2].player_count = 3;
    games_played[2].playTime[0] = 3; games_played[2].playTime[1] = 3; games_played[2].playTime[2] = 3;
    strcpy(games_played[2].start_date, "28-04-2024");
    strcpy(games_played[2].end_date, "");
    games_played[2].is_completed = 0;

    // Session 3
    games_played[3] = (GamePlayed){0};
    games_played[3].session_id = next_session_id++;
    games_played[3].game_id = 6;
    games_played[3].playerIds[0] = 104; games_played[3].playerIds[1] = 105; games_played[3].playerIds[2] = -3;
    games_played[3].player_count = 3;
    games_played[3].scores[0] = 620; games_played[3].scores[1] = 580; games_played[3].scores[2] = 490;
    games_played[3].prizes[0] = 1;
    games_played[3].playTime[0] = 6; games_played[3].playTime[1] = 6; games_played[3].playTime[2] = 6;
    strcpy(games_played[3].start_date, "20-04-2024");
    strcpy(games_played[3].end_date, "26-04-2024");
    games_played[3].is_completed = 1;

    // Session 4 (active)
    games_played[4] = (GamePlayed){0};
    games_played[4].session_id = next_session_id++;
    games_played[4].game_id = 8;
    games_played[4].playerIds[0] = 101; games_played[4].playerIds[1] = 105;
    games_played[4].playerIds[2] = 103; games_played[4].playerIds[3] = 104;
    games_played[4].player_count = 4;
    games_played[4].playTime[0] = 2; games_played[4].playTime[1] = 2; games_played[4].playTime[2] = 2; games_played[4].playTime[3] = 2;
    strcpy(games_played[4].start_date, "29-04-2024");
    strcpy(games_played[4].end_date, "");
    games_played[4].is_completed = 0;

    total_sessions = 5;
    data_dirty = 1; // fresh sample data hasn't been saved yet
}

// ============================================================
// MAIN MENU
// ============================================================
void print_menu() {
    printf("\n============== ONLINE GAME MANAGEMENT SYSTEM ==============\n");
    printf(" 1. Register New Player\n");
    printf(" 2. Add Game Preferences\n");
    printf(" 3. Start Game Session\n");
    printf(" 4. Quit Game Session\n");
    printf(" 5. End Game Session\n");
    printf(" 6. Search Games Played by Others\n");
    printf(" 7. Display Game-wise Inactive Users\n");
    printf(" 8. Display Active Users\n");
    printf(" 9. Display Games with Most Prizes\n");
    printf("10. Display Top 5 Longest Played Games\n");
    printf("11. Display Players of a Game\n");
    printf("12. Remove Inactive Guests\n");
    printf("13. Display All Games\n");
    printf("14. Save Data to File\n");
    printf("15. Reload Data from File\n");
    printf(" 0. Save & Exit\n");
    printf("=============================================================\n");
}

int main() {
    printf("Loading saved data...\n");
    if (!load_data()) {
        printf("No existing save file found. Loading sample data instead.\n");
        initialize_sample_data();
    }

    int choice;
    do {
        print_menu();
        choice = get_int("Enter your choice: ");

        switch (choice) {
            case 1: register_new_player(); break;
            case 2: {
                int playerId = get_int("Enter Player ID: ");
                add_game_preferences(playerId);
                break;
            }
            case 3: start_game_session(); break;
            case 4: quit_game_session(); break;
            case 5: end_game_session(); break;
            case 6: search_games_played_by_others(); break;
            case 7: display_gamewise_inactive_users(); break;
            case 8: display_active_users(); break;
            case 9: display_games_with_most_prizes(); break;
            case 10: display_top5_longest_played_games(); break;
            case 11: display_players_of_game(); break;
            case 12: remove_inactive_guests(); break;
            case 13: display_games(); break;
            case 14: save_data(); break;
            case 15: {
                int confirm = get_int("Reloading will discard any unsaved changes. Continue? (1 = Yes, 0 = No): ");
                if (confirm == 1) {
                    player_count = game_count = total_sessions = guest_count = 0;
                    next_player_id = 101; next_game_id = 1; next_session_id = 1; next_guest_id = 1;
                    if (!load_data()) {
                        printf("No save file found. Loading sample data instead.\n");
                        initialize_sample_data();
                    }
                }
                break;
            }
            case 0:
                if (data_dirty) {
                    printf("Saving unsaved changes...\n");
                    save_data();
                }
                printf("Exiting the system. Goodbye!\n");
                break;
            default:
                printf("Invalid choice! Please enter a number from the menu.\n");
        }
    } while (choice != 0);

    return 0;
}
