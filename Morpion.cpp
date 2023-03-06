#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>

using namespace std;

map<string, vector<float>> actionsRewards;
map<string, vector<int>> actionsCount;

int RANDOM_RATE = 5;
int GAMES_COUNT = 150000;
float LEARNING_RATE = 0.002;
float WIN_REWARD = 1.0;
float DEFEAT_REWARD = 0.0;
float DRAW_REWARD = 0.5;
float INVALID_MOOVE_REWARD = -1.0;
float INIT_VALUE = 0.75;

string createEmptyGrid(int size) {
    return "---------";
}

int getAction(string grid) {
    if (
        rand()%100 < RANDOM_RATE // should use random sampling
        || actionsRewards.find(grid) == actionsRewards.cend() // grid unknown
    ) {
        int randomNumber = rand();
        for (int i=0; i<grid.length(); i++) {
            if (grid[(i + randomNumber) % 9] == '-') {
                return (i + randomNumber) % 9;
            }
        }
        return -1;
    }
    vector<float> rewards = actionsRewards[grid];
    float max = 0.0;
    int index = -1;
    for (int i=0; i<rewards.size(); i++) {
        if ((actionsRewards[grid][i] > max || index == -1) && grid[i] == '-') {
            max = actionsRewards[grid][i];
            index = i;
        }
    }
    return index;
}

char playerWin(string grid) {
    for (int i=0; i<3; i++) {
        if (grid[i] == grid[i+3] && grid[i] == grid[i+6] && grid[i] != '-') {
            return grid[i+3];
        }
        if (grid[i*3] == grid[i*3+1] && grid[i*3] == grid[i*3+2] && grid[i*3] != '-') {
            return grid[i*3];
        }
    }
    if (grid[0] == grid[4] && grid[0] == grid[8] && grid[0] != '-') {
        return grid[0];
    }
    if (grid[2] == grid[4] && grid[2] == grid[6] && grid[2] != '-') {
        return grid[2];
    }
    return '-';
}

void update_weights(float learnrate, vector<string> states, vector<int> values, float score) {
    for (int i=0; i<values.size(); i++) {
        if (actionsRewards.find(states[i]) == actionsRewards.cend()) {
            vector<float> newWeights(9);
            for (int e=0; e<9; e++) {
                if (states[i][e] == '-') {
                    newWeights[e] = INIT_VALUE;
                } else {
                    newWeights[e] = INVALID_MOOVE_REWARD;
                }
            }
            vector<int> newCounts(9);
            actionsCount[states[i]] = newCounts;
            actionsRewards[states[i]] = newWeights;
        }
        actionsCount[states[i]][values[i]] += 1;
        actionsRewards[states[i]][values[i]] = score * learnrate + actionsRewards[states[i]][values[i]]*(1.0-learnrate);
    }
}

string invertGrid(string grid) {
    string ret = "";
    for (int i=0; i<grid.length(); i++) {
        if (grid[i] == '-') {
            ret += '-';
        } else {
            ret += grid[i] == '0' ? '1' : '0'; 
        }
    }
    return ret;
}

void playGame() {
    vector<int> playerActions[2];
    vector<string> playerStates[2];

    string grid = createEmptyGrid(9);
    int iterCounter = 0;
    while (playerWin(grid) == '-' && iterCounter < 9) {
        int player = iterCounter%2;
        string vizGrid = player == 1 ? invertGrid(grid) : string(grid);
        int action = getAction(vizGrid);
        if (action == -1) {
            break;
        }
        playerActions[player].push_back(action);
        playerStates[player].push_back(vizGrid);
        grid = grid.replace(action, 1, to_string(player));
        iterCounter++;
    }
    if (playerWin(grid) != '-') {
        int playerWon = playerWin(grid) == '1' ? 1 : 0;
        update_weights(LEARNING_RATE, playerStates[0], playerActions[0], playerWon == 0 ? WIN_REWARD : DEFEAT_REWARD);
        update_weights(LEARNING_RATE, playerStates[1], playerActions[1], playerWon == 1 ? WIN_REWARD : DEFEAT_REWARD);
    } else {
        // draw
        update_weights(LEARNING_RATE, playerStates[0], playerActions[0], DRAW_REWARD);
        update_weights(LEARNING_RATE, playerStates[1], playerActions[1], DRAW_REWARD);
    }
}

void print() {
    map<string, vector<float>>::iterator it;
    for (it = actionsRewards.begin(); it != actionsRewards.end(); it++)
    {
        cout << it->first << " : ";
        for (int i=0; i<9; i++) {
            cout << actionsRewards[it->first][i] << "(" << actionsCount[it->first][i] << ") , ";
        }
        cout << "\n";
    }
}

void saveData(string path) {
    ofstream myfile;
    myfile.open(path);
    string rawText = "grid,0,1,2,3,4,5,6,7,8\n";

    map<string, vector<float>>::iterator it;
    for (it = actionsRewards.begin(); it != actionsRewards.end(); it++)
    {
        rawText += it->first + ",";
        for (int i=0; i<9; i++) {
            rawText += to_string(actionsRewards[it->first][i]) + ",";
        }
        rawText += "\n";
    }
    myfile << rawText;
    myfile.close();
}

string getGridValues(string id) {
    string rawText = "";
    for (int i=0; i<9; i++) {
        rawText += to_string(actionsRewards[id][i]) + ";";
    }
    return rawText;
}

void save2file(string path, string text) {
    ofstream myfile;
    myfile.open(path);
    myfile << text;
    myfile.close();
}

int main() {
    string paramsEval = "top-left;top-center;top-right;middle-left;middle-center;middle-right;bottom-left;bottom-center;bottom-right\n";
    for (int i=0; i<GAMES_COUNT; i++) {
        playGame();
        paramsEval += getGridValues("---------") + "\n";
    }

    save2file("./evolution.csv", paramsEval);

    saveData("./datas.csv");
    cout << actionsRewards.size() << " grids learned";
}