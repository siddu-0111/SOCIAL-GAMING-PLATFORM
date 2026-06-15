#include <stdio.h>
#include <string.h>

#define MAX_PLAYERS 100
#define MAX_GAMES 100
#define MAX_GAMES_PLAYED 100
#define MAX_GUESTS 100
#define MAX_PREFS 10

// Structures
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
    int playerIds[10];
    int player_count;
    int scores[10];
    int prizes[10];
    int playTime[10];
    char start_date[20];
    char end_date[20];
    int is_completed;
    int players_quit[10];
} GamePlayed;

typedef struct {
    int guest_id;
    char name[50];
    char start_date[20];
    int active_days;
    int isBlocked;
} GuestUser;

// Array of structures
Player players[MAX_PLAYERS];
GameMaster games[MAX_GAMES];
GamePlayed games_played[MAX_GAMES_PLAYED];
GuestUser guests[MAX_GUESTS];

// Counters
int player_count = 0;
int game_count = 0;
int total_sessions = 0;
int guest_count = 0;

// Next Available IDs
int next_player_id = 101;
int next_game_id = 1;
int next_session_id = 1;
int next_guest_id = 1;

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

// Finding player by ID
int find_player_by_id(int playerId) {
    for (int i = 0; i < player_count; i++) {
        if (players[i].player_id == playerId) {
            return i;
        }
    }
    return -1;
}

// Finding game by ID
int find_game_by_id(int gameId) {
    for (int i = 0; i < game_count; i++) {
        if (games[i].game_id == gameId) {
            return i;
        }
    }
    return -1;
}

// find guest by ID
int find_guest_by_id(int guestId) {
    for (int i = 0; i < guest_count; i++) {
        if (guests[i].guest_id == guestId) {
            return i;
        }
    }
    return -1;
}

// find session by ID
int find_session_by_id(int sessionId) {
    for (int i = 0; i < total_sessions; i++) {
        if (games_played[i].session_id == sessionId) {
            return i;
        }
    }
    return -1;
}

// Count active games for a player
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

// calculating Prizes for a game
int calculate_game_prizes(int gameId, int score) {
    int gameIndex = find_game_by_id(gameId);
    if (gameIndex == -1) {
        return 0;
    }
    // More score means more prizes
    if (score >= 1000) {
        return 5; // First prize
    } else if (score >= 500) {
        return 3; // Second prize
    } else if (score >= 100) {
        return 1; // Third prize
    }
    return 0; // No prize
}

void register_new_player(){
    if(player_count >= MAX_PLAYERS){
        printf("Player database full!\n");
        return;
    }
    Player new_player;
    new_player.player_id = next_player_id++;
    printf("Enter player name: ");
    scanf("%s", new_player.name);

    printf("Enter registration date (DD-MM-YYYY): ");
    scanf("%s", new_player.registration_date);

    // Initializing stats
    new_player.stats.games_played = 0;
    new_player.stats.games_won = 0;
    new_player.stats.total_score = 0;
    new_player.stats.prizes = 0;

    new_player.pref_count = 0; // No preferences initially

    // Find Games with Most Playtime
    if(game_count > 0) {
        // Create a temporary array of games for sorting
        GameMaster tempGames[MAX_GAMES];
        for(int i = 0; i < game_count; i++) {
            tempGames[i] = games[i];
        }

        // Sort games by totalPlayTime in descending order (bubble sort)
        for(int i = 0; i < game_count - 1; i++) {
            for(int j = 0; j < game_count - i - 1; j++) {
                if(tempGames[j].totalPlayTime < tempGames[j + 1].totalPlayTime) {
                    GameMaster temp = tempGames[j];
                    tempGames[j] = tempGames[j + 1];
                    tempGames[j + 1] = temp;
                }
            }
        }

        // Assign top 3 games with most playtime as preferences
        int gamesToAssign = (game_count < 3) ? game_count : 3;
        for(int i = 0; i < gamesToAssign; i++) {
            if(new_player.pref_count < MAX_PREFS) {
                new_player.game_preferences[new_player.pref_count++] = tempGames[i].game_id;
            }
        }

        // Print assigned games
        if(new_player.pref_count > 0) {
            printf("Popular game preferences assigned (based on playtime): \n");
            for(int i = 0; i < new_player.pref_count; i++) {
                int gid = new_player.game_preferences[i];
                int gindex = find_game_by_id(gid);
                if(gindex != -1) {
                    printf("- %s (ID: %d) - Total Play Time: %d days\n", 
                           games[gindex].name, games[gindex].game_id, games[gindex].totalPlayTime);
                }
            }
            printf("\n");
        }
        else {
            printf("No games available to assign preferences.\n");
        }
    }
    else {
        printf("\n No games available in the system! Add game preferences later.\n");
    }
    players[player_count++] = new_player;
    printf("Player registered successfully with ID: %d\n", new_player.player_id);
}

// display all games
void display_games() {
    printf("\n Available Games : \n");
    for(int i = 0; i < game_count; i++) {
        printf("ID: %d | %s | Type: %s | Players Required: %d\n",
               games[i].game_id,
               games[i].name,
               games[i].type == 1 ? "Single" : "Multiple",
               games[i].players_required);
        printf("\n");
    }
}

// manually add game preferences to a player 
void add_game_preferences(int playerId) {
    int playerIndex = find_player_by_id(playerId);
    if (playerIndex == -1) {
        printf("Player not found!\n");
        return;
    }

    if(players[playerIndex].pref_count >= MAX_PREFS) {
        printf("Maximum game preferences reached!\n");
        return;
    }

    display_games();

    int gameId;
    printf("Enter Game ID to add to preferences (0 to stop): ");
    scanf("%d", &gameId);

    if(gameId == 0) return;

    if(find_game_by_id(gameId) == -1) {
        printf("Invalid Game ID!\n");
        return;
    }

    // if already in preferences
    for(int i = 0; i < players[playerIndex].pref_count; i++) {
        if(players[playerIndex].game_preferences[i] == gameId) {
            printf("Game already in preferences!\n");
            return;
        }
    }

    
    players[playerIndex].game_preferences[players[playerIndex].pref_count] = gameId;
    players[playerIndex].pref_count++;
    printf("Game added to preferences successfully!\n");
}

// Starting New Game Session 
void start_game_session() {
    if(total_sessions >= MAX_GAMES_PLAYED) {
        printf("Maximum game sessions reached!\n");
        return;
    }
    GamePlayed new_session;
    new_session.session_id = next_session_id++;
    
    printf("Enter Game ID : ");
    int game_id;
    scanf("%d", &game_id);
    int gameIndex = find_game_by_id(game_id);

    if(gameIndex == -1) {
        printf("Game not found!\n");
        return;
    }

    new_session.game_id = game_id;

    printf("Enter session start date (DD-MM-YYYY): ");
    scanf("%s", new_session.start_date);

    strcpy(new_session.end_date, "");
    new_session.player_count = 0;
    new_session.is_completed = 0;

    for(int i = 0; i < 10; i++) {
        new_session.players_quit[i] = 0; 
        // 0 = not quit, 1 = quit
    }

    printf("\nAdding players to the session:\n");
    printf("Enter Player ID (0 for guest, -1 to stop):\n");

    while(new_session.player_count < games[gameIndex].players_required) {
        printf("Player %d: ", new_session.player_count + 1);
        int playerId;
        scanf("%d", &playerId);

        if(playerId == -1) break;

        if(playerId == 0) {
            // Register guest user to a session
            if(guest_count >= MAX_GUESTS) {
                printf("Guest user database full!\n");
                continue;
            }
            GuestUser new_guest;
            new_guest.guest_id = next_guest_id++;
            printf("Enter guest name: ");
            scanf("%s", new_guest.name);
            strcpy(new_guest.start_date, new_session.start_date);
            new_guest.active_days = 0;
            new_guest.isBlocked = 0;

            guests[guest_count++] = new_guest;
            playerId = -new_guest.guest_id;
            // checking negative for guest

            printf("Guest '%s' created with ID: %d\n", new_guest.name, new_guest.guest_id);
        }
        else {
            int playerIndex = find_player_by_id(playerId);
            if(playerIndex == -1) {
                printf("Player not found! Please check the Player ID.\n");
                continue;
            }
        }

        // Check if Guest is blocked
        if(playerId < 0) {
            int guestIndex = find_guest_by_id(-playerId);
            if(guestIndex != -1 && guests[guestIndex].isBlocked) {
                printf("This guest is currently blocked.\n");
                continue;
            }
        }

        // Adding player to session
        new_session.playerIds[new_session.player_count] = playerId;
        new_session.scores[new_session.player_count] = 0;
        new_session.prizes[new_session.player_count] = 0;
        new_session.playTime[new_session.player_count] = 0;
        new_session.players_quit[new_session.player_count] = 0; //player not quit
        new_session.player_count++;

        // Update player's active games count
        if(playerId > 0) {
            int playerIndex = find_player_by_id(playerId);
            if(playerIndex != -1) {
                players[playerIndex].stats.games_played++;
            }
        }

        // Check if required players reached
        if(new_session.player_count >= games[gameIndex].players_required) {
            printf("Required number of players reached for the session.\n");
            break;
        }
    }

    if(new_session.player_count == 0) {
        printf("No players added to the session. Session not created.\n");
        return;
    }

    games_played[total_sessions++] = new_session;

    printf("Game session started successfully with Session ID: %d\n", new_session.session_id);
    printf("Players in this session:\n");
    printf("Game %s (ID: %d)\n", games[gameIndex].name, new_session.game_id);
    printf("--------------------------------------------\n");
    for(int i = 0; i < new_session.player_count; i++) {
        int pid = new_session.playerIds[i];
        if(pid > 0) {
            int pIndex = find_player_by_id(pid);
            if(pIndex != -1) {
                printf("Player: %s (ID: %d)\n", players[pIndex].name, pid);
            }
        }
        else {
            int gIndex = find_guest_by_id(-pid);
            if(gIndex != -1) {
                printf("Guest: %s (ID: %d)\n", guests[gIndex].name, -pid);
            }
        }
    }
    printf("--------------------------------------------\n");
    printf("Total Players: %d\n", new_session.player_count);
}

// Quit Game Session 
void quit_game_session() {
    printf("\n----- QUIT GAME SESSION ----\n");

    // Show active sessions
    printf("Active Game Sessions:\n");
    printf("Session ID  Game ID  Game Name        Players  Start Date\n");
    printf("----------  -------  ---------------  -------  ----------\n");

    int activeSessionsFound = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (!games_played[i].is_completed) {
            int gameIndex = find_game_by_id(games_played[i].game_id);
            if (gameIndex != -1) {
                printf("%-10d  %-7d  %-15s  %-7d  %s\n",
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

    printf("\nEnter Session ID to quit: ");
    int sessionId;
    scanf("%d", &sessionId);

    int sessionIndex = find_session_by_id(sessionId);

    if (sessionIndex == -1) {
        printf("Session not found!\n");
        return;
    }

    if (games_played[sessionIndex].is_completed) {
        printf("This session is already completed!\n");
        return;
    }

    GamePlayed session = games_played[sessionIndex];
    int gameIndex = find_game_by_id(session.game_id);

    printf("\nPlayers in this session: %d (%s) \n", sessionId, games[gameIndex].name);
    printf("Index  Player ID   Name/Guest Name   Quit Status\n");
    printf("------------------------------------------------\n");
    for(int i = 0; i < session.player_count; i++) {
        int playerId = session.playerIds[i];
        char status[10] = "Active";
        if(session.players_quit[i] == 1) {
            strcpy(status, "Quit");
        }

        if(playerId > 0) {
            int playerIndex = find_player_by_id(playerId);
            if(playerIndex != -1) {
                printf("%-5d  %-10d  %-15s  %-10s\n", i + 1, playerId, players[playerIndex].name, status);
            }
        }
        else {
            int guestIndex = find_guest_by_id(-playerId);
            if(guestIndex != -1) {
                printf("%-5d  %-10d  %-15s  %-10s\n", i + 1, playerId, guests[guestIndex].name, status);
            }
        }
    }

    printf("\nEnter Player Index to quit (1 to %d): ", session.player_count);
    int playerIndexToQuit;
    scanf("%d", &playerIndexToQuit);

    if(playerIndexToQuit < 1 || playerIndexToQuit > session.player_count) {
        printf("Invalid player index!\n");
        return;
    }

    if(session.players_quit[playerIndexToQuit - 1] == 1) {
        printf("This player has already quit the session!\n");
        return;
    }

    int quittingPlayerId = session.playerIds[playerIndexToQuit - 1];

    if(quittingPlayerId < 0) {
        int guestIndex = find_guest_by_id(-quittingPlayerId);
        if(guestIndex != -1) {
            printf("Guest '%s' (ID: %d) has quit the session.\n", guests[guestIndex].name, -quittingPlayerId);
        }
    }
    else {
        int playerIndex = find_player_by_id(quittingPlayerId);
        if(playerIndex != -1) {
            printf("Player '%s' (ID: %d) has quit the session.\n", players[playerIndex].name, quittingPlayerId);
        }
    }

    printf(" Are you sure you want to quit? (1 = Yes, 0 = No): ");
    int confirm;
    scanf("%d", &confirm);
    if(confirm != 1) {
        printf("Quit action cancelled.\n");
        return;
    }

    // Marking player as quit
    games_played[sessionIndex].players_quit[playerIndexToQuit - 1] = 1;
    printf("Player has been marked as quit in the session.\n");

    // Updating player statistics
    if(quittingPlayerId > 0) {
        int playerIndex = find_player_by_id(quittingPlayerId);
        if(playerIndex != -1) {
            players[playerIndex].stats.games_played--;
            printf("Game count updated for player '%s'.\n", players[playerIndex].name);
        }
    }

    // No need to update guest stats for quitting

    int allQuit = 1;
    for(int i = 0; i < session.player_count; i++) {
        if(games_played[sessionIndex].players_quit[i] == 0) {
            allQuit = 0;
            break;
        }
    }

    if(allQuit) {
        printf("All players have quit the session.\n");
        games_played[sessionIndex].is_completed = 1;
        printf("Session marked as completed.\n");
        char currentDate[20];
        printf("Enter session end date (DD-MM-YYYY): ");
        scanf("%s", currentDate);
        strcpy(games_played[sessionIndex].end_date, currentDate);
    }

    printf("Quit operation completed.\n");
}

// Ending Game Session 
void end_game_session() {
    printf("\n=== END GAME SESSION ===\n");
    
    // Show active sessions
    printf("Active Game Sessions:\n");
    printf("Session ID  Game ID  Game Name        Players  Start Date\n");
    printf("----------  -------  ---------------  -------  ----------\n");
    
    int activeSessionsFound = 0;
    for (int i = 0; i < total_sessions; i++) {
        if (!games_played[i].is_completed) {
            int gameIndex = find_game_by_id(games_played[i].game_id);
            printf("%-11d %-7d %-15s %-7d %-10s\n",
                   games_played[i].session_id,
                   games_played[i].game_id,
                   games[gameIndex].name,
                   games_played[i].player_count,
                   games_played[i].start_date);
            activeSessionsFound = 1;
        }
    }
    
    if (!activeSessionsFound) {
        printf("No active game sessions found.\n");
        return;
    }
    
    printf("\nEnter Session ID to end: ");
    int sessionId;
    scanf("%d", &sessionId);
    
    int sessionIndex = find_session_by_id(sessionId);
    
    if (sessionIndex == -1) {
        printf("Session not found!\n");
        return;
    }
    
    if (games_played[sessionIndex].is_completed) {
        printf("This session is already completed!\n");
        return;
    }
    
    GamePlayed session = games_played[sessionIndex];
    
    // Collect scores and playtime for each player
    char endDate[20];
    int validDate = 0;

    while(!validDate) {
        printf("Enter session end date (DD-MM-YYYY): ");
        scanf("%s", endDate);
        
        int daysActive = days_difference(session.start_date, endDate);
        if(daysActive < 0) {
            printf("End date cannot be before start date. Please re-enter.\n");
        } else {
            validDate = 1;
            strcpy(games_played[sessionIndex].end_date, endDate);

            // Calculate days active
            if(daysActive == 0) {
                daysActive = 1; // Minimum 1 day
            }
            printf("Enter scores for each player :\n");
            int totalPrizesAwarded = 0;
            int bestScore = -1;
            int winningPlayerId = -1;
            int sessionTotalPlayTime = daysActive;

            for(int i = 0; i < session.player_count;i++){
                int playerId = session.playerIds[i];
                
                // Skip players who quit
                if(session.players_quit[i] == 1){
                    printf("Player ");
                    if(playerId > 0){
                        int playerIndex = find_player_by_id(playerId);
                        if(playerIndex != -1){
                            printf("%s has quit the session. Skipping score entry.\n", players[playerIndex].name);
                        }
                    } else {
                        int guestIndex = find_guest_by_id(-playerId);
                        if(guestIndex != -1){
                            printf("Guest %s has quit the session. Skipping score entry.\n", guests[guestIndex].name);
                        }
                    }
                    printf(" Quit the session - score set to 0\n");
                    games_played[sessionIndex].scores[i] = 0;
                    games_played[sessionIndex].prizes[i] = 0;
                    games_played[sessionIndex].playTime[i] = 0;
                    continue;
                }

                printf("Enter Score for ");
                if(playerId > 0){
                    int playerIndex = find_player_by_id(playerId);
                    if(playerIndex != -1){
                        printf("Player %s (ID: %d): ", players[playerIndex].name, playerId);
                    }
                } else {
                    int guestIndex = find_guest_by_id(-playerId);
                    if(guestIndex != -1){
                        printf("Guest %s (ID: %d): ", guests[guestIndex].name, -playerId);
                    }
                }

                games_played[sessionIndex].scores[i] = 0;
                scanf("%d", &games_played[sessionIndex].scores[i]);

                // Checking if Guest has been played for more than 15 days
                if(playerId < 0 && daysActive > 15){
                    printf(" ⚠ Guest players cannot play more than 15 days. Limiting playtime to 15 days.\n");
                    printf("Setting playtime for Guest ID %d to 15 days and blocking \n", -playerId);
                    
                    games_played[sessionIndex].playTime[i] = 15;
                    int guestIndex = find_guest_by_id(-playerId);

                    if(guestIndex != -1){
                        guests[guestIndex].active_days = 15;
                        guests[guestIndex].isBlocked = 1;
                        printf(" 🚫 Guest %s (ID: %d) has been blocked due to excessive playtime.\n", guests[guestIndex].name, -playerId);
                    }
                } else {
                    games_played[sessionIndex].playTime[i] = daysActive;
                }

                // Calculating prizes
                games_played[sessionIndex].prizes[i] = calculate_game_prizes(session.game_id, games_played[sessionIndex].scores[i]);
                totalPrizesAwarded += games_played[sessionIndex].prizes[i];

                // Track best score for awards
                if(games_played[sessionIndex].scores[i] > bestScore && session.players_quit[i] == 0){
                    bestScore = games_played[sessionIndex].scores[i];
                    winningPlayerId = playerId;
                }

                printf("Active days: %d, Prizes earned: %d\n", games_played[sessionIndex].playTime[i], games_played[sessionIndex].prizes[i]);

                // Update player/guest statistics
                if(playerId > 0){
                    int playerIndex = find_player_by_id(playerId);
                    if(playerIndex != -1){
                        players[playerIndex].stats.total_score += games_played[sessionIndex].scores[i];
                        players[playerIndex].stats.prizes += games_played[sessionIndex].prizes[i];
                    }
                } else {
                    int guestIndex = find_guest_by_id(-playerId);
                    if(guestIndex != -1 && !guests[guestIndex].isBlocked){
                        guests[guestIndex].active_days += daysActive;

                        // Block guest if active for 20+ days
                        if(guests[guestIndex].active_days >= 20){
                            guests[guestIndex].isBlocked = 1;
                            printf(" 🚫 Guest %s (ID: %d) has been blocked due to excessive total playtime.\n", guests[guestIndex].name, -playerId);
                        }
                    }
                }
            }

            // Awarding game winner
            if(winningPlayerId > 0){
                int winnerIndex = find_player_by_id(winningPlayerId);
                if(winnerIndex != -1){
                    players[winnerIndex].stats.games_won++;
                    printf("🏆 Player %s (ID: %d) is the winner of this session with a score of %d!\n", players[winnerIndex].name, winningPlayerId, bestScore);
                }
            }

            // Update game statistics
            int gameIndex = find_game_by_id(session.game_id);
            if(gameIndex != -1){
                games[gameIndex].prizesAwarded += totalPrizesAwarded;
                games[gameIndex].totalPlayTime += sessionTotalPlayTime;
            }

            // Mark session as completed
            games_played[sessionIndex].is_completed = 1;

            printf(" ✅ Game session %d ended successfully.\n", sessionId);
            printf("Total Prizes Awarded in this session: %d\n", totalPrizesAwarded);
            printf("Session duration : %d days\n", daysActive);
        }
    }
}

// Search Operations 
void search_games_played_by_others(){
    int playerId, gameId;
    printf("Enter Player ID to search for: ");
    scanf("%d", &playerId);
    printf("Enter Game ID to search for: ");
    scanf("%d", &gameId);

    // First, check if the player exists
    int playerIndex = find_player_by_id(playerId);
    if (playerIndex == -1) {
        printf("Player with ID %d not found!\n", playerId);
        return;
    }

    // Check if the game exists
    int gameIndex = find_game_by_id(gameId);
    if (gameIndex == -1) {
        printf("Game with ID %d not found!\n", gameId);
        return;
    }

    char gameName[50];
    strcpy(gameName, games[gameIndex].name);
    char playerName[50];
    strcpy(playerName, players[playerIndex].name);

    printf("\n=== PLAYERS WHO PLAYED %s (Game ID %d) ===\n", gameName, gameId);
    printf("Player %s (ID: %d) wants to see who else played this game and what other games they play\n\n", playerName, playerId);
    
    // First, find all players who played the specified game
    int foundPlayers[MAX_PLAYERS];
    int foundGuests[MAX_GUESTS];
    int playerCount = 0;
    int guestCount = 0;
    
    printf("Players who played %s:\n", gameName);
    printf("Session ID  Player ID   Type       Player/Guest Name  Score \n");
    printf("----------  ---------   ---------  ----------------   ----- \n");

    int foundGamePlayers = 0;
    for(int i = 0; i < total_sessions; i++) {
        if(games_played[i].game_id == gameId && games_played[i].is_completed) {
            for(int j = 0; j < games_played[i].player_count; j++) {
                int pid = games_played[i].playerIds[j];
                
                // Show all players EXCEPT the specified player
                if(pid != playerId) {
                    if(pid > 0){
                        int pIndex = find_player_by_id(pid);
                        if(pIndex != -1){
                            printf("%-11d  %-9d  %-9s  %-16s  %-5d\n",
                                   games_played[i].session_id,
                                   pid,
                                   "Player",
                                   players[pIndex].name,
                                   games_played[i].scores[j]);
                            foundPlayers[playerCount++] = pid;
                            foundGamePlayers = 1;
                        }
                    }
                    else{
                        int gIndex = find_guest_by_id(-pid);
                        if(gIndex != -1){
                            printf("%-11d  %-9d  %-9s  %-16s  %-5d\n",
                                   games_played[i].session_id,
                                   pid,
                                   "Guest",
                                   guests[gIndex].name,
                                   games_played[i].scores[j]);
                            foundGuests[guestCount++] = -pid;
                            foundGamePlayers = 1;
                        }
                    }
                }
            }
        }
    }

    if(!foundGamePlayers) {
        printf("No other players found who played '%s' (Game ID: %d).\n", gameName, gameId);
        return;
    }

    // Now show other games played by these players
    printf("\n=== OTHER GAMES PLAYED BY THESE PLAYERS ===\n");
    
    int foundOtherGames = 0;
    
    // Check for regular players
    for(int i = 0; i < playerCount; i++) {
        int currentPlayerId = foundPlayers[i];
        int currentPlayerIndex = find_player_by_id(currentPlayerId);
        
        if(currentPlayerIndex != -1) {
            printf("\nPlayer: %s (ID: %d) also plays:\n", 
                   players[currentPlayerIndex].name, currentPlayerId);
            
            int playerGamesFound = 0;
            
            // Check player's game preferences
            if(players[currentPlayerIndex].pref_count > 0) {
                printf("  Game Preferences: ");
                for(int j = 0; j < players[currentPlayerIndex].pref_count; j++) {
                    int prefGameId = players[currentPlayerIndex].game_preferences[j];
                    int prefGameIndex = find_game_by_id(prefGameId);
                    if(prefGameIndex != -1 && prefGameId != gameId) {
                        printf("%s (ID: %d) ", games[prefGameIndex].name, prefGameId);
                        playerGamesFound = 1;
                        foundOtherGames = 1;
                    }
                }
                printf("\n");
            }
            
            // Check actual game sessions
            printf("  Games Played: ");
            int sessionGamesFound = 0;
            for(int j = 0; j < total_sessions; j++) {
                if(games_played[j].is_completed) {
                    for(int k = 0; k < games_played[j].player_count; k++) {
                        if(games_played[j].playerIds[k] == currentPlayerId && 
                           games_played[j].game_id != gameId) {
                            int sessionGameIndex = find_game_by_id(games_played[j].game_id);
                            if(sessionGameIndex != -1) {
                                printf("%s (Score: %d) ", 
                                       games[sessionGameIndex].name, 
                                       games_played[j].scores[k]);
                                sessionGamesFound = 1;
                                foundOtherGames = 1;
                            }
                        }
                    }
                }
            }
            if(!sessionGamesFound) {
                printf("No other games played");
            }
            printf("\n");
        }
    }

    // Check for guest players
    for(int i = 0; i < guestCount; i++) {
        int currentGuestId = foundGuests[i];
        int currentGuestIndex = find_guest_by_id(currentGuestId);
        
        if(currentGuestIndex != -1) {
            printf("\nGuest: %s (ID: %d) also played:\n", 
                   guests[currentGuestIndex].name, currentGuestId);
            
            int guestGamesFound = 0;
            printf("  Games Played: ");
            for(int j = 0; j < total_sessions; j++) {
                if(games_played[j].is_completed) {
                    for(int k = 0; k < games_played[j].player_count; k++) {
                        if(games_played[j].playerIds[k] == -currentGuestId && 
                           games_played[j].game_id != gameId) {
                            int sessionGameIndex = find_game_by_id(games_played[j].game_id);
                            if(sessionGameIndex != -1) {
                                printf("%s (Score: %d) ", 
                                       games[sessionGameIndex].name, 
                                       games_played[j].scores[k]);
                                guestGamesFound = 1;
                                foundOtherGames = 1;
                            }
                        }
                    }
                }
            }
            if(!guestGamesFound) {
                printf("No other games played");
            }
            printf("\n");
        }
    }

    if(!foundOtherGames) {
        printf("\nNo other games found for these players.\n");
    }
}

// Game wise inactive users
void display_gamewise_inactive_users(){
    printf("\nGame-wise Inactive Users:\n");
    printf("Game ID  Game Name    Player ID     Player Name\n");
    printf("-------  ---------    ---------     -----------\n");

    int found = 0;
    for(int i = 0 ; i < game_count; i++){
        if(!games_played[i].is_completed){
            int gIndex = find_game_by_id(games_played[i].game_id);
            if(gIndex != -1){
                for(int j = 0; j < games_played[i].player_count; j++){
                    if(games_played[i].players_quit[j] == 0){
                        int pid = games_played[i].playerIds[j];
                        if(pid > 0){
                            int pIndex = find_player_by_id(pid);
                            if(pIndex != -1){
                                printf("%-7d  %-11s  %-11d  %-13s\n",
                                       games[gIndex].game_id,
                                       games[gIndex].name,
                                       pid,
                                       players[pIndex].name);
                                found = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    if(!found){
        printf("No inactive users found.\n");
    }
}

// Active users
void display_active_users(){
    int k;
    printf("Enter minimum number of active games (K): ");
    scanf("%d", &k);

    printf("\nPlayers with more than %d active games:\n", k);
    printf("Player ID   Player Name       Active Games\n");
    printf("---------   ---------------   ------------\n");

    int found = 0;
    for(int i = 0; i < player_count; i++) {
        int activeGames = count_player_active_games(players[i].player_id);
        if(activeGames > k) {
            printf("%-10d  %-15s  %-12d\n",
                   players[i].player_id,
                   players[i].name,
                   activeGames);
            found = 1;
        }
    }

    if(!found) {
        printf("No players found with more than %d active games.\n", k);
    }
}

// Sorting Operations 
void display_games_with_most_prizes(){
    // Creating a copy of games array for sorting
    GameMaster tempGames[MAX_GAMES];
    for(int i = 0; i < game_count; i++) {
        tempGames[i] = games[i];
    }

    // Bubble Sort based on prizesAwarded (Reverse order)
    for(int i = 0; i < game_count - 1; i++) {
        for(int j = 0; j < game_count - i - 1; j++) {
            if(tempGames[j].prizesAwarded < tempGames[j + 1].prizesAwarded) {
                GameMaster temp = tempGames[j];
                tempGames[j] = tempGames[j + 1];
                tempGames[j + 1] = temp;
            }
        }
    }

    printf("\nGames with Most Prizes Awarded:\n");
    printf("Game ID  Game Name        Prizes Awarded\n");
    printf("-------  ---------------  --------------\n");

    for(int i = 0; i < game_count; i++) {
        printf("%-7d  %-15s  %-14d\n",
               tempGames[i].game_id,
               tempGames[i].name,
               tempGames[i].prizesAwarded);
    }
}

// Top 5 Longest Played Games
void display_top5_longest_played_games(){
    // Creating a copy of games array for sorting
    GameMaster tempGames[MAX_GAMES];
    for(int i = 0; i < game_count; i++) {
        tempGames[i] = games[i];
    }

    // Bubble Sort based on totalPlayTime (Reverse order)
    for(int i = 0; i < game_count - 1; i++) {
        for(int j = 0; j < game_count - i - 1; j++) {
            if(tempGames[j].totalPlayTime < tempGames[j + 1].totalPlayTime) {
                GameMaster temp = tempGames[j];
                tempGames[j] = tempGames[j + 1];
                tempGames[j + 1] = temp;
            }
        }
    }

    printf("\nTop 5 Longest Played Games:\n");
    printf("Game ID  Game Name        Total Play Time (days)\n");
    printf("-------  ---------------  ----------------------\n");

    int topCount = game_count < 5 ? game_count : 5;
    for(int i = 0; i < topCount; i++) {
        printf("%-7d  %-15s  %-22d\n",
               tempGames[i].game_id,
               tempGames[i].name,
               tempGames[i].totalPlayTime);
    }
}

// Players of Game G
void display_players_of_game(){
    int gameId;
    printf("Enter Game ID : ");
    scanf("%d", &gameId);

    // Collecting players who played the game
    int playingPlayers[MAX_PLAYERS];
    int playerScores[MAX_PLAYERS];
    int playingCount = 0;

    for(int i = 0; i < total_sessions; i++) {
        if(games_played[i].game_id == gameId && !games_played[i].is_completed) {
            for(int j = 0; j < games_played[i].player_count; j++) {
                if(games_played[i].players_quit[j] == 0 && games_played[i].playerIds[j] > 0){
                    playingPlayers[playingCount] = games_played[i].playerIds[j];
                    playerScores[playingCount] = games_played[i].scores[j];
                    playingCount++;
                }
            }
        }        
    }

    if(playingCount == 0) {
        printf("No active players found for Game ID %d.\n", gameId);
        return;
    }

    // Sort players by scores (Bubble Sort)
    for(int i = 0; i < playingCount - 1; i++) {
        for(int j = 0; j < playingCount - i - 1; j++) {
            if(playerScores[j] < playerScores[j + 1]) {
                // Swap scores
                int tempScore = playerScores[j];
                playerScores[j] = playerScores[j + 1];
                playerScores[j + 1] = tempScore;

                // Swap player IDs
                int tempId = playingPlayers[j];
                playingPlayers[j] = playingPlayers[j + 1];
                playingPlayers[j + 1] = tempId;
            }
        }
    }

    printf("\nPlayers currently playing Game ID %d:\n", gameId);
    printf("Player ID   Player Name       Score\n");
    printf("---------   ---------------   -----\n");
    for(int i = 0; i < playingCount; i++) {
        int playerIndex = find_player_by_id(playingPlayers[i]);
        if(playerIndex != -1) {
            printf("%-10d  %-15s  %-5d\n",
                   playingPlayers[i],
                   players[playerIndex].name,
                   playerScores[i]);
        }
    }
}

// Delete Operations
void remove_inactive_guests(){
    printf("\nRemoving guests active for more than 20 days...\n");

    int removedCount = 0;
    for(int i = guest_count -1; i >= 0; i--) {
        if(guests[i].active_days > 20) {
            printf("Removing Guest %s (ID: %d) - Active Days: %d\n",
                   guests[i].name,
                   guests[i].guest_id,
                   guests[i].active_days);

            // Remove guest from all game sessions
            for(int j = 0; j < total_sessions; j++) {
                for(int k = 0; k < games_played[j].player_count; k++) {
                    if(games_played[j].playerIds[k] == -guests[i].guest_id) {
                        // Mark as quit
                        games_played[j].players_quit[k] = 1;
                        printf(" - Marked as quit in Session ID %d\n", games_played[j].session_id);
                    }
                }
            }
            // Shift remaining guests
            for(int j = i; j < guest_count - 1; j++) {
                guests[j] = guests[j + 1];
            }
            guest_count--;
            removedCount++;
        }
    }

    printf("Total guests removed: %d\n", removedCount);
}

void initialize_sample_data() {
    // Games - 10 different games
    games[0].game_id = next_game_id++;
    strcpy(games[0].name, "Chess");
    games[0].type = 1;
    games[0].players_required = 1;
    games[0].prizesAwarded = 25;
    games[0].totalPlayTime = 120;

    games[1].game_id = next_game_id++;
    strcpy(games[1].name, "Free Fire");
    games[1].type = 2;
    games[1].players_required = 50;
    games[1].prizesAwarded = 45;
    games[1].totalPlayTime = 85;

    games[2].game_id = next_game_id++;
    strcpy(games[2].name, "Among Us");
    games[2].type = 2;
    games[2].players_required = 4;
    games[2].prizesAwarded = 30;
    games[2].totalPlayTime = 200;

    games[3].game_id = next_game_id++;
    strcpy(games[3].name, "Temple Run");
    games[3].type = 1;
    games[3].players_required = 1;
    games[3].prizesAwarded = 20;
    games[3].totalPlayTime = 75;

    games[4].game_id = next_game_id++;
    strcpy(games[4].name, "Call of Duty");
    games[4].type = 2;
    games[4].players_required = 10;
    games[4].prizesAwarded = 35;
    games[4].totalPlayTime = 60;

    games[5].game_id = next_game_id++;
    strcpy(games[5].name, "Subway Surfers");
    games[5].type = 1;
    games[5].players_required = 1;
    games[5].prizesAwarded = 28;
    games[5].totalPlayTime = 95;

    games[6].game_id = next_game_id++;
    strcpy(games[6].name, "Ludo");
    games[6].type = 2;
    games[6].players_required = 4;
    games[6].prizesAwarded = 50;
    games[6].totalPlayTime = 180;

    games[7].game_id = next_game_id++;
    strcpy(games[7].name, "Sudoku");
    games[7].type = 1;
    games[7].players_required = 1;
    games[7].prizesAwarded = 22;
    games[7].totalPlayTime = 70;

    games[8].game_id = next_game_id++;
    strcpy(games[8].name, "BGMI");
    games[8].type = 2;
    games[8].players_required = 100;
    games[8].prizesAwarded = 40;
    games[8].totalPlayTime = 150;

    games[9].game_id = next_game_id++;
    strcpy(games[9].name, "Candy Crush");
    games[9].type = 1;
    games[9].players_required = 1;
    games[9].prizesAwarded = 18;
    games[9].totalPlayTime = 45;

    game_count = 10;

    // Sample players with games played
    players[0].player_id = next_player_id++;
    strcpy(players[0].name, "Raj");
    strcpy(players[0].registration_date, "15-01-2024");
    players[0].stats.games_played = 8;  
    players[0].stats.games_won = 5;     
    players[0].stats.total_score = 2500; 
    players[0].stats.prizes = 15;       
    // Game preferences
    players[0].game_preferences[0] = 1; 
    players[0].game_preferences[1] = 3; 
    players[0].game_preferences[2] = 6; 
    players[0].game_preferences[3] = 8; 
    players[0].pref_count = 4;

    players[1].player_id = next_player_id++;
    strcpy(players[1].name, "Ram");
    strcpy(players[1].registration_date, "20-02-2024");
    players[1].stats.games_played = 12; 
    players[1].stats.games_won = 8;     
    players[1].stats.total_score = 3200; 
    players[1].stats.prizes = 20;       
    // Game preferences
    players[1].game_preferences[0] = 2;
    players[1].game_preferences[1] = 4; 
    players[1].game_preferences[2] = 7; 
    players[1].game_preferences[3] = 9; 
    players[1].game_preferences[4] = 5; 
    players[1].pref_count = 5;

    players[2].player_id = next_player_id++;
    strcpy(players[2].name, "Dev");
    strcpy(players[2].registration_date, "10-03-2024");
    players[2].stats.games_played = 6;  
    players[2].stats.games_won = 3;     
    players[2].stats.total_score = 1800; 
    players[2].stats.prizes = 8;        
    // Game preferences
    players[2].game_preferences[0] = 5; 
    players[2].game_preferences[1] = 8; 
    players[2].game_preferences[2] = 1; 
    players[2].game_preferences[3] = 4; 
    players[2].pref_count = 4;

    // Add more players with games
    players[3].player_id = next_player_id++;
    strcpy(players[3].name, "Sri");
    strcpy(players[3].registration_date, "05-04-2024");
    players[3].stats.games_played = 4;
    players[3].stats.games_won = 2;
    players[3].stats.total_score = 900;
    players[3].stats.prizes = 4;
    // Game preferences
    players[3].game_preferences[0] = 3; 
    players[3].game_preferences[1] = 6; 
    players[3].game_preferences[2] = 9; 
    players[3].pref_count = 3;

    players[4].player_id = next_player_id++;
    strcpy(players[4].name, "Sai");
    strcpy(players[4].registration_date, "12-04-2024");
    players[4].stats.games_played = 10;
    players[4].stats.games_won = 6;
    players[4].stats.total_score = 2800;
    players[4].stats.prizes = 18;
    // Game preferences
    players[4].game_preferences[0] = 2;
    players[4].game_preferences[1] = 7; 
    players[4].game_preferences[2] = 1; 
    players[4].game_preferences[3] = 8; 
    players[4].game_preferences[4] = 4; 
    players[4].pref_count = 5;

    player_count = 5;

    // Sample guests
    guests[0].guest_id = next_guest_id++;
    strcpy(guests[0].name, "Siva");
    strcpy(guests[0].start_date, "01-04-2024");
    guests[0].active_days = 12;
    guests[0].isBlocked = 0;

    guests[1].guest_id = next_guest_id++;
    strcpy(guests[1].name, "Dil");
    strcpy(guests[1].start_date, "05-04-2024");
    guests[1].active_days = 25;
    guests[1].isBlocked = 1;

    // Add more guests
    guests[2].guest_id = next_guest_id++;
    strcpy(guests[2].name, "Nag");
    strcpy(guests[2].start_date, "10-04-2024");
    guests[2].active_days = 8;
    guests[2].isBlocked = 0;

    guest_count = 3;

    // Sample game sessions
    games_played[0].session_id = next_session_id++;
    games_played[0].game_id = 1; 
    games_played[0].playerIds[0] = 101; 
    games_played[0].playerIds[1] = 102; 
    games_played[0].player_count = 2;
    games_played[0].scores[0] = 850;
    games_played[0].scores[1] = 720;
    games_played[0].prizes[0] = 3;
    games_played[0].prizes[1] = 1;
    games_played[0].playTime[0] = 5;
    games_played[0].playTime[1] = 5;
    strcpy(games_played[0].start_date, "10-04-2024");
    strcpy(games_played[0].end_date, "15-04-2024");
    games_played[0].is_completed = 1;
    games_played[0].players_quit[0] = 0;
    games_played[0].players_quit[1] = 0;

    games_played[1].session_id = next_session_id++;
    games_played[1].game_id = 3; 
    games_played[1].playerIds[0] = 101; 
    games_played[1].playerIds[1] = 103; 
    games_played[1].playerIds[2] = -1; 
    games_played[1].player_count = 3;
    games_played[1].scores[0] = 450;
    games_played[1].scores[1] = 380;
    games_played[1].scores[2] = 290;
    games_played[1].prizes[0] = 1;
    games_played[1].prizes[1] = 0;
    games_played[1].prizes[2] = 0;
    games_played[1].playTime[0] = 8;
    games_played[1].playTime[1] = 8;
    games_played[1].playTime[2] = 8;
    strcpy(games_played[1].start_date, "18-04-2024");
    strcpy(games_played[1].end_date, "26-04-2024");
    games_played[1].is_completed = 1;
    games_played[1].players_quit[0] = 0;
    games_played[1].players_quit[1] = 0;
    games_played[1].players_quit[2] = 0;

    games_played[2].session_id = next_session_id++;
    games_played[2].game_id = 2; 
    games_played[2].playerIds[0] = 102; 
    games_played[2].playerIds[1] = 103; 
    games_played[2].playerIds[2] = -2; 
    games_played[2].player_count = 3;
    games_played[2].scores[0] = 0;
    games_played[2].scores[1] = 0;
    games_played[2].scores[2] = 0;
    games_played[2].prizes[0] = 0;
    games_played[2].prizes[1] = 0;
    games_played[2].prizes[2] = 0;
    games_played[2].playTime[0] = 3;
    games_played[2].playTime[1] = 3;
    games_played[2].playTime[2] = 3;
    strcpy(games_played[2].start_date, "28-04-2024");
    strcpy(games_played[2].end_date, "");
    games_played[2].is_completed = 0;
    games_played[2].players_quit[0] = 0;
    games_played[2].players_quit[1] = 0;
    games_played[2].players_quit[2] = 0;

    // Add more game sessions
    games_played[3].session_id = next_session_id++;
    games_played[3].game_id = 6; 
    games_played[3].playerIds[0] = 104; 
    games_played[3].playerIds[1] = 105; 
    games_played[3].playerIds[2] = -3; 
    games_played[3].player_count = 3;
    games_played[3].scores[0] = 620;
    games_played[3].scores[1] = 580;
    games_played[3].scores[2] = 490;
    games_played[3].prizes[0] = 1;
    games_played[3].prizes[1] = 0;
    games_played[3].prizes[2] = 0;
    games_played[3].playTime[0] = 6;
    games_played[3].playTime[1] = 6;
    games_played[3].playTime[2] = 6;
    strcpy(games_played[3].start_date, "20-04-2024");
    strcpy(games_played[3].end_date, "26-04-2024");
    games_played[3].is_completed = 1;
    games_played[3].players_quit[0] = 0;
    games_played[3].players_quit[1] = 0;
    games_played[3].players_quit[2] = 0;

    games_played[4].session_id = next_session_id++;
    games_played[4].game_id = 8; 
    games_played[4].playerIds[0] = 101; 
    games_played[4].playerIds[1] = 105; 
    games_played[4].playerIds[2] = 103; 
    games_played[4].playerIds[3] = 104; 
    games_played[4].player_count = 4;
    games_played[4].scores[0] = 0;
    games_played[4].scores[1] = 0;
    games_played[4].scores[2] = 0;
    games_played[4].scores[3] = 0;
    games_played[4].prizes[0] = 0;
    games_played[4].prizes[1] = 0;
    games_played[4].prizes[2] = 0;
    games_played[4].prizes[3] = 0;
    games_played[4].playTime[0] = 2;
    games_played[4].playTime[1] = 2;
    games_played[4].playTime[2] = 2;
    games_played[4].playTime[3] = 2;
    strcpy(games_played[4].start_date, "29-04-2024");
    strcpy(games_played[4].end_date, "");
    games_played[4].is_completed = 0;
    games_played[4].players_quit[0] = 0;
    games_played[4].players_quit[1] = 0;
    games_played[4].players_quit[2] = 0;
    games_played[4].players_quit[3] = 0;

    total_sessions = 5;
}

// MAIN MENU
int main() {
    int choice;
    initialize_sample_data();

    do {
        printf("\n=== ONLINE GAME MANAGEMENT SYSTEM ===\n");
        printf("1. Register New Player\n");
        printf("2. Add Game Preferences\n");
        printf("3. Start Game Session\n");
        printf("4. Quit Game Session\n");
        printf("5. End Game Session\n");
        printf("6. Search Games Played by Others\n");
        printf("7. Display Game-wise Inactive Users\n");
        printf("8. Display Active Users\n");
        printf("9. Display Games with Most Prizes\n");
        printf("10. Display Top 5 Longest Played Games\n");
        printf("11. Display Players of a Game\n");
        printf("12. Remove Inactive Guests\n");
        printf("13. Display All Games\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch(choice) {
            case 1:
                register_new_player();
                break;
            case 2:
                {
                    int playerId;
                    printf("Enter Player ID: ");
                    scanf("%d", &playerId);
                    add_game_preferences(playerId);
                }
                break;
            case 3:
                start_game_session();
                break;
            case 4:
                quit_game_session();
                break;
            case 5:
                end_game_session();
                break;
            case 6:
                search_games_played_by_others();
                break;
            case 7:
                display_gamewise_inactive_users();
                break;
            case 8:
                display_active_users();
                break;
            case 9:
                display_games_with_most_prizes();
                break;
            case 10:
                display_top5_longest_played_games();
                break;
            case 11:
                display_players_of_game();
                break;
            case 12:
                remove_inactive_guests();
                break;
            case 13:
                display_games();
                break;
            case 0:
                printf("Exiting the system.!\n");
                break;
            default:
                printf("Invalid choice! Please try again.\n");
        }
    } while(choice != 0);
    
    return 0;
}
