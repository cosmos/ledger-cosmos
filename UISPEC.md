UI Specification
-------------------------

##Terminology:

**Left click** - clicking and releasing left button

**Right click** - clicking and releasing right button

**Double click** - clicking and releasing both left and right 

**Left hold** - clicking and holding left button

**Right hold** - clicking and holding right button

**Double hold** - clicking and holding both left and right 

## UI pages:
### A. 'Welcome' page

#### Layout:
Screen 1. (default)
```
LINE1:
LINE2:                      Tendermint
LINE3: [tendermint icon]    Cosmos TEST!   [down icon]
```

Screen 2.
```
LINE1:
LINE2:  [up icon]             About         [down icon]
LINE3:
```

Screen 3.
```
LINE1:
LINE2: [up icon] [quit icon] Quit app
LINE3:
```
#### Interface:
##### Left click
Moves to the previous screen.
##### Right click
Moves to the next screen.
##### Left and Right clicked together on the Screen #3
Exits the app.
 
### B. 'Received Transaction' page

#### Layout:
Screen 1. (default)
```
LINE1:
LINE2:            View transaction    [down icon]
LINE3:
```
Screen 2.
```
LINE1:
LINE2: [up icon]   Sign transaction    [down icon]
LINE3:
```

Screen 3.
```
LINE1:
LINE2: [up icon]   [reject icon] Reject
LINE3:
```

#### Interface:
##### Left click
Moves to the previous screen.
##### Right click
Moves to the next screen.
##### Double click on the Screen #1
Switches UI to the 'View Transaction' page
##### Double click on the Screen #2
- signs transaction
- returns signed transaction to the client app
- switches UI to the 'Welcome' page
##### Double click on the Screen #3
- rejects transaction 
- switches UI to the 'Welcome' page

### C. 'View Transaction' page

#### Layout:

Screen (template)
```
LINE1: [left icon]    SECP256K1 - 0n/0k         [right icon]
LINE2:             title (json element key)
LINE3:            value (json element value)
```

Line1 contains the page info - the signature type (here SECP256K1) followed by the page index (n) and the total number of pages (k).

Line2 contains the title - key element from the json transaction for a particular page.

Line3 contains the value - value element from the json transaction for a particular page.

#####Here's the page breakdown for a simple json transaction:
Screen 1. (default)
```
LINE1: [left icon]  SECP256K1 - 01/08         [right icon]
LINE2:                chain_id
LINE3:              test-chain-1
```

Screen 2. 
```
LINE1: [left icon]  SECP256K1 - 02/08         [right icon]
LINE2:                   sequences
LINE3:                      [1]
```

Screen 3. 
```
LINE1: [left icon]  SECP256K1 - 03/08         [right icon]
LINE2:                  fee_bytes
LINE3: {"amount":[{"denom":"photon", "amount":5}], "gas":10000}
```

Screen 4. 
```
LINE1: [left icon]    SECP256K1 - 03/08       [right icon]
LINE2:                  fee_bytes
LINE3: {"amount":[{"denom":"photon", "amount":5}], "gas":10000}
```

Screen 5. 
```
LINE1: [left icon]     SECP256K1 - 04/08      [right icon]
LINE2:            msg_bytes/inputs/address
LINE3:                69FE2314BAC34EF
```

Screen 6. 
```
LINE1: [left icon]     SECP256K1 - 05/08      [right icon]
LINE2:            msg_bytes/inputs/coins
LINE3:                69FE2314BAC34EF
```

Screen 7. 
```
LINE1: [left icon]     SECP256K1 - 06/08      [right icon]
LINE2:            msg_bytes/outputs/address
LINE3:                69FE2314BAC34EF
```

Screen 8. 
```
LINE1: [left icon]     SECP256K1 - 07/08      [right icon]
LINE2:            msg_bytes/outputs/coins
LINE3:                69FE2314BAC34EF
```

Screen 9. 
```
LINE1: [left icon]     SECP256K1 - 08/08      [right icon]
LINE2:                     alt_bytes
LINE3:                        null
```

####Scrolling
Ledger screen can only fit around 20 characters and therefore we need a way for displaying the key and the value strings that are longer. Here we assume that the page info will always fit the screen line.

Ledger supports the smooth pixel scrolling which is enabled by default for the line #3 (i.e. the line that holds the json value). If a text is longer than (around) 20 characters, then Ledger will automatically start scrolling the line.

We are currently using 256 bytes long buffer to keep the value string. If the value read from the json transaction is longer than that, then we split it into 256 chunks and display and scroll each chunk individually. 

Here's an example:

Screen 3. 
```
LINE1: [left icon]       SECP256K1 - 03/08         [right icon]
LINE2:                   fee_bytes - 01/02
LINE3:            {"amount":[{"denom":"photon", "a [first 256 bytes of the value]
```

Screen 3a. 
```
LINE1: [left icon]       SECP256K1 - 03/08         [right icon]
LINE2:                   fee_bytes - 02/02
LINE3:               mount":5}], "gas":10000}     [the next 256 bytes of the value]
```

We'll use term: 'Value Chunk Preview' to describe this mode.

#### Interface:
##### Left click
Moves to the previous screen, unless the current screen is #1, in which case it switches back to the 'Received Transaction' page.
##### Right click
Moves to the next screen, unless the current screen is the last one, in which case it switches back to the 'Received Transaction' page.
##### Double click
Switches back to the 'View Transaction' page
##### Left hold
Same as Left click, unless we are in the 'Value Chunk Preview', in which case we step out from the 'Value Chunk Preview' and skip to the previous screen.
##### Right hold
Same as Right click, unless we are in the 'Value Chunk Preview', in which case we step out from the 'Value Chunk Preview' and skip to the next screen.

##Known issues:
We currently don't scroll the key (in line 2) and only display its first 20 characters. The key can be longer though.
Unfortunately scrolling 2 lines at the same time seems to be impossible in Ledger (TODO: confirm this).
We need a way of scrolling the key or some other way of displaying the full key string.

Here's the proposed solution:

```
IF key is longer than 20 characters
THEN 
    scroll the KEY in line 2
    IF value in line 3 is longer than 20 characters
    THEN
        display "DBL-CLICK TO VIEW" in line 3
    ELSE
        display the VALUE in line 3
```

Screen 'Scrolling Key'. 
```
LINE1: [left icon]       SECP256K1 - 03/08         [right icon]
LINE2:                ...scrolling long key... 
LINE3:                   DBL-CLICK TO VIEW     
```
     
In this mode we have to double click to switch to the VALUE preview.
In VALUE preview mode we scroll the VALUE in line 3 and only 
display the last 20 characters for the KEY.
```
LINE1: [left icon]       SECP256K1 - 03/08         [right icon]
LINE2:                   cropped_key 02/03
LINE3:                  ...scrolling value....    
```