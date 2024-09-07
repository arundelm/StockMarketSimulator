# Market Simulator
A C++ stock market simulator that processes buy and sell orders from multiple traders across multiple stocks, matching orders based on price and share quantity. The system includes support for tracking trader performance, market trends, and verbose output for transaction logs.

## Features
**Trader Transactions:** Tracks all buy and sell orders placed by traders, including the number of shares and their corresponding prices.

**Order Matching:** Matches buy and sell orders for the same stock at the same price, ensuring fair and accurate transactions.

**Market Watcher:** Monitors stocks for significant price changes (spikes or drops) using a state-based system.

**Trader Evaluation:** Tracks each trader's net position (long or short) and displays their total buys, sells, and remaining shares.

### Command-Line Options:
**Verbose Mode (-v):** Outputs detailed transaction logs for each trade executed.

**Trader Evaluation Mode (-t):** Outputs a report of all traders and their positions at the end of the simulation.

**Market Watcher Mode (-w):** Outputs a report of stocks that experienced significant price changes.

### How It Works
The Market class simulates a real-time stock market where multiple traders buy and sell shares of various stocks. Buy and sell orders are matched based on price and quantity. If no matching order is available, the order is queued until a match is found.



