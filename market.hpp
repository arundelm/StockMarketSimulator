#include <functional>
#include <ostream>
#include <queue>
#include <getopt.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include "P2random.h"
using namespace std;

class Market {
public:
    enum class State {
        Initial, SeenOne, SeenBoth, MaybeBetter
    };

    struct Watcher {
        struct Trade {
            uint32_t time;
            uint32_t value;
        };
        State current = State::Initial;
        Trade one;
        Trade two;
        Trade maybe;
    };

    struct Trader {
        int sellAmount;
        int buyAmount;
        int totalShares;
    };

    struct TradeOrder {
        uint32_t value;
        uint32_t shareAmount;
        uint32_t trader;
        uint32_t id;
    };

    Market(const int argc, char * argv[]) {
        getMode(argc, argv);
        readInput();
    }

    void readInput() {
        string labels;
        string inputMode;
        getline(cin, labels);
        cin >> labels;
        cin >> inputMode;

        cin >> labels;
        cin >> numTraders;
        traders.resize(numTraders);

        cin >> labels;
        cin >> numStocks;
        stocks.resize(numStocks);

        string type;
        uint32_t time;
        uint32_t value, numShares;
        uint32_t traderIndex;
        uint32_t stockIndex;
        uint32_t lastTime = 0;
        char garbage;

        stringstream ss;
        if(inputMode == "PR"){
            uint32_t seed;
            uint32_t numOrders;
            uint32_t arrivalRate;
            cin >> labels; cin >> seed;
            cin >> labels; cin >> numOrders;
            cin >> labels; cin >> arrivalRate;
            P2random::PR_init(ss, seed, numTraders, numStocks, numOrders, arrivalRate);
        }

        istream &inputStream = inputMode == "PR" ? ss : cin;

        cout << "Processing trades...\n";
        while(inputStream >> time >> type >> garbage >> traderIndex >> garbage >> stockIndex >> garbage >> value >> garbage >> numShares){
            if(time < lastTime){
                cerr << "Invalid decreasing timestamp" << endl;
                exit(1);
            }
            lastTime = time;

            // Updates current time
            if(time != currentTime){
                if(isMedian){
                    for(size_t i = 0; i < stocks.size(); ++i){
                        if(stocks[i].tradeExecuted){
                            cout << "Median trade value on stock " << i
                            << " at time " << currentTime << " is " << stocks[i].m.median() << ".\n";
                        }
                    }
                }
                currentTime = time;
            }

            if(traderIndex >= numTraders){
                cerr << "Invalid trader id" << endl;
                exit(1);
            }
            if(stockIndex >= numStocks){
                cerr << "Invalid stock id" << endl;
                exit(1);
            }
        
            if(value <= 0){
                cerr << "Invalid trade value" << endl;
                exit(1);
            }
            if(numShares <= 0){
                cerr << "Invalid number of shares" << endl;
                exit(1);
            } 

            traders[traderIndex].totalShares += numShares;
            TradeOrder t = {value, numShares, traderIndex, currentID};
            ++currentID;
            if(type == "SELL"){

                if(isWatcher){
                    
                    if(stocks[stockIndex].fillBuy.current == State::Initial){
                        stocks[stockIndex].fillBuy.one = {time, value};
                        stocks[stockIndex].fillBuy.current = State::SeenOne;
                    }
                    if(stocks[stockIndex].fillBuy.current == State::SeenOne){
                        if(value > stocks[stockIndex].fillBuy.one.value){
                            stocks[stockIndex].fillBuy.one = {time, value};
                        }
                    }
                    if(stocks[stockIndex].fillBuy.current == State::SeenBoth){
                        if(stocks[stockIndex].fillBuy.one.value < value){
                            stocks[stockIndex].fillBuy.maybe = {time, value}; 
                            stocks[stockIndex].fillBuy.current = State::MaybeBetter;
                        }
                    }
                    if(stocks[stockIndex].fillBuy.current == State::MaybeBetter){
                        if(value > stocks[stockIndex].fillBuy.maybe.value){
                            stocks[stockIndex].fillBuy.maybe = {time, value};
                        }
                    }

                    if(stocks[stockIndex].fillSell.current == State::SeenOne){
                        if(value >= stocks[stockIndex].fillSell.one.value){
                            stocks[stockIndex].fillSell.two = {time, value};
                            stocks[stockIndex].fillSell.current = State::SeenBoth;
                        }
                    }
                    if(stocks[stockIndex].fillSell.current == State::SeenBoth){
                        if(stocks[stockIndex].fillSell.two.value < value){
                            stocks[stockIndex].fillSell.two = {time, value};
                        }
                    }
                    if(stocks[stockIndex].fillSell.current == State::MaybeBetter){
                        if(value >= stocks[stockIndex].fillSell.maybe.value){
                            if((value - stocks[stockIndex].fillSell.maybe.value) > (stocks[stockIndex].fillSell.two.value - stocks[stockIndex].fillSell.one.value)){
                                stocks[stockIndex].fillSell.one = stocks[stockIndex].fillSell.maybe;
                                stocks[stockIndex].fillSell.two = {time, value};
                                stocks[stockIndex].fillSell.current = State::SeenBoth;
                            }
                        }
                    }  
                }

                stocks[stockIndex].sellers.push(t);
                traders[traderIndex].sellAmount += numShares;
                TradeOrder currentSeller = stocks[stockIndex].sellers.top();

                while(true){
                    TradeOrder currentBuyer = {0, 0, 0, 0};
                    if(!stocks[stockIndex].buyers.empty()){
                        currentBuyer = stocks[stockIndex].buyers.top();
                        if(currentBuyer.value == currentSeller.value){
                            uint32_t sharesTraded = 0;
                            uint32_t currSeller = currentSeller.trader;
                             uint32_t currBuyer = currentBuyer.trader;
                            if(currentSeller.shareAmount > currentBuyer.shareAmount){
                                TradeOrder swap;
                                stocks[stockIndex].buyers.pop();
                                swap.shareAmount = currentSeller.shareAmount - currentBuyer.shareAmount;
                                swap.value = currentSeller.value;
                                swap.trader = currentSeller.trader;
                                swap.id = currentSeller.id;
                                stocks[stockIndex].sellers.pop();
                                stocks[stockIndex].sellers.push(swap);
                                traders[currentBuyer.trader].totalShares -= currentBuyer.shareAmount;
                                traders[currentSeller.trader].totalShares -= currentBuyer.shareAmount;
                                sharesTraded = currentBuyer.shareAmount;
                                currentSeller = swap;
                            } else if(currentSeller.shareAmount < currentBuyer.shareAmount){
                                TradeOrder swap;
                                stocks[stockIndex].sellers.pop();
                                swap.shareAmount = currentBuyer.shareAmount - currentSeller.shareAmount;
                                swap.value = currentBuyer.value;
                                swap.trader = currentBuyer.trader;
                                swap.id = currentBuyer.id;
                                stocks[stockIndex].buyers.pop();
                                stocks[stockIndex].buyers.push(swap);
                                traders[currentBuyer.trader].totalShares -= currentSeller.shareAmount;
                                traders[currentSeller.trader].totalShares -= currentSeller.shareAmount;
                                sharesTraded = currentSeller.shareAmount;
                                currentSeller = {0, 0, 0, 0};
                            } else if(currentSeller.shareAmount == currentBuyer.shareAmount){
                                stocks[stockIndex].sellers.pop();
                                stocks[stockIndex].buyers.pop();
                                traders[currentBuyer.trader].totalShares -= currentBuyer.shareAmount;
                                traders[currentSeller.trader].totalShares -= currentSeller.shareAmount;
                                sharesTraded = currentSeller.shareAmount;
                                currentSeller = {0, 0, 0, 0};
                            }

                            if(isVerbose){
                                cout << "Trader " << currSeller << " sold " << sharesTraded <<  " shares of stock " << stockIndex << " to Trader " <<  currBuyer << ".\n";
                            }
    
                            
                            if(isMedian){
                                stocks[stockIndex].m.push(sharesTraded * 2);
                                stocks[stockIndex].tradeExecuted = true;
                            }
                            ++numTransactions;
                        }
                    }

                    if(currentSeller.shareAmount == 0 || currentBuyer.shareAmount == 0 || currentSeller.value < currentBuyer.value || currentSeller.value > currentBuyer.value){
                        break;
                    }
                }
        
            } else if(type == "BUY"){

                if(isWatcher){
                    
                    if(stocks[stockIndex].fillSell.current == State::Initial){
                        stocks[stockIndex].fillSell.one = {time, value};
                        stocks[stockIndex].fillSell.current = State::SeenOne;
                    }
                    if(stocks[stockIndex].fillSell.current == State::SeenOne){
                        if(value < stocks[stockIndex].fillSell.one.value){
                            stocks[stockIndex].fillSell.one = {time, value};
                        }
                    }
                    if(stocks[stockIndex].fillSell.current == State::SeenBoth){
                        if(stocks[stockIndex].fillSell.one.value > value){
                            stocks[stockIndex].fillSell.maybe = {time, value}; 
                            stocks[stockIndex].fillSell.current = State::MaybeBetter;
                        }
                    }
                    if(stocks[stockIndex].fillSell.current == State::MaybeBetter){
                        if(value < stocks[stockIndex].fillSell.maybe.value){
                            stocks[stockIndex].fillSell.maybe = {time, value};
                        }
                    }

                    if(stocks[stockIndex].fillBuy.current == State::SeenOne){
                        if(value <= stocks[stockIndex].fillBuy.one.value){
                            stocks[stockIndex].fillBuy.two = {time, value};
                            stocks[stockIndex].fillBuy.current = State::SeenBoth;
                        }
                    }
                    if(stocks[stockIndex].fillBuy.current == State::SeenBoth){
                        if(stocks[stockIndex].fillBuy.two.value > value){
                            stocks[stockIndex].fillBuy.two = {time, value};
                        }
                    }
                    if(stocks[stockIndex].fillBuy.current == State::MaybeBetter){
                        if(stocks[stockIndex].fillBuy.maybe.value > value){
                            if((stocks[stockIndex].fillBuy.maybe.value - value) > (stocks[stockIndex].fillBuy.one.value - stocks[stockIndex].fillBuy.two.value)){
                                stocks[stockIndex].fillBuy.one = stocks[stockIndex].fillBuy.maybe;
                                stocks[stockIndex].fillBuy.two = {time, value};
                                stocks[stockIndex].fillBuy.current = State::SeenBoth;
                            }
                        }
                    } 
                }
    
                stocks[stockIndex].buyers.push(t);
                traders[traderIndex].buyAmount += numShares;
                TradeOrder currentBuyer = stocks[stockIndex].buyers.top();

                while(true){

                    TradeOrder currentSeller = {0, 0, 0, 0};
                    if(!stocks[stockIndex].sellers.empty()){
                        currentSeller = stocks[stockIndex].sellers.top();
                        if(currentBuyer.value == currentSeller.value){
                            uint32_t currSeller = currentSeller.trader;
                            uint32_t currBuyer = currentBuyer.trader;
                            uint32_t sharesTraded = 0;
                            if(currentSeller.shareAmount > currentBuyer.shareAmount){
                                TradeOrder swap;
                                stocks[stockIndex].buyers.pop();
                                swap.shareAmount = currentSeller.shareAmount - currentBuyer.shareAmount;
                                swap.value = currentSeller.value;
                                swap.trader = currentSeller.trader;
                                swap.id = currentSeller.id;
                                stocks[stockIndex].sellers.pop();
                                stocks[stockIndex].sellers.push(swap);
                                traders[currentBuyer.trader].totalShares -= currentBuyer.shareAmount;
                                traders[currentSeller.trader].totalShares -= currentBuyer.shareAmount;
                                sharesTraded = currentBuyer.shareAmount;
                                currentSeller = swap;
                            } else if(currentSeller.shareAmount < currentBuyer.shareAmount){
                                TradeOrder swap;
                                stocks[stockIndex].sellers.pop();
                                swap.shareAmount = currentBuyer.shareAmount - currentSeller.shareAmount;
                                swap.value = currentBuyer.value;
                                swap.trader = currentBuyer.trader;
                                swap.id = currentBuyer.id;
                                stocks[stockIndex].buyers.pop();
                                stocks[stockIndex].buyers.push(swap);
                                traders[currentBuyer.trader].totalShares -= currentSeller.shareAmount;
                                traders[currentSeller.trader].totalShares -= currentSeller.shareAmount;
                                sharesTraded = currentSeller.shareAmount;
                                currentSeller = {0, 0, 0, 0};
                            } else if(currentSeller.shareAmount == currentBuyer.shareAmount){
                                stocks[stockIndex].sellers.pop();
                                stocks[stockIndex].buyers.pop();
                                traders[currentBuyer.trader].totalShares -= currentBuyer.shareAmount;
                                traders[currentSeller.trader].totalShares -= currentSeller.shareAmount;
                                sharesTraded = currentSeller.shareAmount;
                                currentBuyer = {0, 0, 0, 0};
                            }

                            if(isVerbose){
                                cout << "Trader " << currSeller << " sold " << sharesTraded <<  " shares of stock " 
                                << stockIndex << " to Trader " <<  currBuyer << ".\n";

                            }
                            if(isMedian){
                                stocks[stockIndex].m.push(sharesTraded);
                                stocks[stockIndex].tradeExecuted = true;
                            }
                            ++numTransactions;
                        }
                    } else {
                        break;
                    }

                    if(currentSeller.shareAmount == 0 || currentBuyer.shareAmount == 0 || currentSeller.value < currentBuyer.value || currentSeller.value > currentBuyer.value){
                        break;
                    }
                }

            }

        }

        if(isMedian){
            for(size_t i = 0; i < stocks.size(); ++i){
                if(stocks[i].tradeExecuted){
                    cout << "Median trade value on stock " << i
                    << " at time " << currentTime << " is " << stocks[i].m.median() << ".\n";
                }
            }
        }

        cout << "---End of Day---\n";
        cout << "Transactions: " << numTransactions << "\n";

        
        if(isTraderEval){
            cout << "---Trader Evaluation---\n";
            for(size_t i = 0; i < traders.size(); ++i){
                int shares = traders[i].buyAmount - traders[i].sellAmount;
                cout << "Trader " << i << " executed " << traders[i].buyAmount << " buy orders and " << traders[i].sellAmount
                 << " sell orders, with ";
                if(shares < 0){
                    cout << "a net short position of " << shares << " shares.\n";
                }else if(shares > 0){
                    cout << "a net long position of " << shares << " shares.\n";
                }else{
                    cout << "a net zero position with 0 shares.\n";
                }
            }
        }

        if(isWatcher){
            cout << "---Market Watcher---\n";
            for(size_t i = 0; i < stocks.size(); ++i){
                if(( stocks[i].fillBuy.current == State::SeenBoth || stocks[i].fillBuy.current == State::MaybeBetter) && ((stocks[i].fillBuy.one.value - stocks[i].fillBuy.two.value) != 0)){
                    cout << "A market watcher would see an price spike on stock " << i << " with a price increase of " 
                    << (stocks[i].fillBuy.one.value - stocks[i].fillBuy.two.value) << ".\n";
                } else {
                    cout << "A market watcher would not see a price spike on stock " << i << ".\n"; 
                }

                if((stocks[i].fillSell.current == State::SeenBoth || stocks[i].fillSell.current == State::MaybeBetter)&&((stocks[i].fillSell.two.value - stocks[i].fillSell.one.value) != 0)){
                    cout << "A market watcher would see a price drop on stock " << i << " with a price decrease of " 
                    << (stocks[i].fillSell.two.value - stocks[i].fillSell.one.value) << ".\n";
                } else {
                    cout << "A market watcher would not see a price drop on stock " << i << ".\n"; 
                }

            }
        }
        
    }

    void getMode(const int argc, char* argv[]) {
        opterr = false;
        int choice = 0;
        int index = 0;
        option long_options[] = {
            { "verbose",       no_argument, nullptr,  'v'},
            { "median",       no_argument, nullptr,  'm'},
            {"trader-eval", no_argument, nullptr,  't'},
            {"watcher", no_argument, nullptr,  'w'}, 
            {  "help",       no_argument, nullptr,  'h'},
            { nullptr,                 0, nullptr, '\0'},
        };

        while ((choice = getopt_long(argc, argv, "vmtwh", long_options, &index)) != -1) {
            switch (choice) {
            case 'h':
                exit(0);

            case 'v': {
                isVerbose = true;
                break;
            }

            case 'm': {
                isMedian = true;
                break;
            }

            case 't': {
                isTraderEval = true;
                break;
            }

            case 'w': {
                isWatcher = true;
                break;
            }

            default:
                cerr << "Unknown command line option" << endl;
                exit(1);
            }
        }
    }

    class Stock {
        friend class Market;

        struct sellerCompare {
            bool operator()(const TradeOrder &a, const TradeOrder &b) const {
                if(a.value < b.value){
                    return true;
                } else if(a.value == b.value){
                    return a.id > b.id;
                }
                return false;
            }
        };

        struct buyerCompare {
            bool operator()(const TradeOrder &a, const TradeOrder &b) const {
                if(a.value > b.value){
                    return true;
                } else if(a.value == b.value){
                   return a.id > b.id;               
                }
                return false;
            }
        };
        
        class Median {
            friend class Stock;
            public:
                
                void push(uint32_t val){
                    if(higherNums.empty()){
                        higherNums.push(val);
                        return;
                    } else {

                        if(val >= higherNums.top()){
                            higherNums.push(val);
                        } else {
                            lowerNums.push(val);
                        }

                    }

                    if(lowerNums.size() - higherNums.size() == 2){
                        higherNums.push(lowerNums.top());
                        lowerNums.pop();
                    } else if(higherNums.size() - lowerNums.size() == 2){
                        lowerNums.push(higherNums.top());
                        higherNums.pop();
                    }
                }

                uint32_t median(){
                    if(higherNums.size() == lowerNums.size()){
                        return ( lowerNums.top() + higherNums.top() ) / 2;
                    } else if(lowerNums.size() > higherNums.size()){
                        return lowerNums.top();
                    } else {
                        return higherNums.top();
                    }
                }

            private:
                priority_queue<uint32_t, vector<uint32_t>, greater<> > higherNums;
                priority_queue<uint32_t, vector<uint32_t>, less<> > lowerNums;

        };

        private:
            priority_queue<TradeOrder, vector<TradeOrder>, sellerCompare> sellers;
            priority_queue<TradeOrder, vector<TradeOrder>, buyerCompare> buyers;
            Median m;
            Watcher fillSell;
            Watcher fillBuy;
            bool tradeExecuted = false;
    };

private:
    vector<Stock> stocks;
    vector<Trader> traders;
    uint32_t numTraders = 0;
    uint32_t numStocks = 0;
    uint32_t numTransactions = 0;
    uint32_t currentTime = 0;
    uint32_t currentID = 0;
    bool isVerbose = false;
    bool isMedian = false;
    bool isTraderEval = false;
    bool isWatcher = false;

    
};

