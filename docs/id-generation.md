# [`EPF_PersistenceIdGenerator`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EPF_PersistenceIdGenerator.c;1)
For persistence a specialized version of the [`EDF_DbEntityIdGenerator`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/EDF_DbEntityIdGenerator.c;1) is used. This is because the persistence system needs to account for multiple hives and deterministic GUIDs for baked map objects.

The format is still a GUID but depending on what it is generated for the exact format is different

## Dynamic entities and scripted states
| 647399a8     | - | 0000      | - | 0017     | - | 0001          | - | 5487         | a9003c24 |
|--------------|---|-----------|---|----------|---|---------------|---|--------------|----------|
| TTTTTTTT     |   | SEQ1      |   | SEQ1     |   | HHHH          |   | RND1         | RND1RND2 |

- `TTTTTTTT` UNIX UTC 32-bit date time stamp. Gives the ID general lexograpical sortability by creation time. Minimizes collisions as each ID becomes unique per second.
- `SEQ1` gives guaranteed uniqueness to the ID due to the combination with the timestamp. Starts at 0 per session. Is incremented by one for each generated ID. There can be skips in the sequence number making their way to the database as not all generated ids will end up being used.
- `HHHH` is the hive index with values between (inclusive) `1` and `4095`.
- `RND1` and `RND2` aim to minimize predictability by malicious actors by introducing pseudo-random numbers. These are however **NOT** cryptographically safe!

## Baked map entities
| 00bb0001     | - | 1a79      | - | 8da7     | - | 3f92          | - | 4c9a         | 1f3582d7 |
|--------------|---|-----------|---|----------|---|---------------|---|--------------|----------|
| 00CCHHHH     |   | SPL1      |   | SPL1     |   | SPL2          |   | SPL2         | SPL3SPL3 |

- `00` The double leading zero lexicographically sorts baked objects as earliest created and gives a hint that the entity in question is in fact baked.
- `CC` is the code of the special GUID format, `bb` is the code for baked map ojects
- `HHHH` is the hive index with values between (inclusive) `1` and `4095`.
- `SPL1`, `SPL2`, and `SPL3` are splits of the deterministic string hash, for the unique identifier, that represents the baked entity (s. [baked entities](baked-entities.md#identification) for details). 

## Root entity collection
| 00ec0001     | - | 0000      | - | 0000     | - | 0000          | - | 0000         | 00000000 |
|--------------|---|-----------|---|----------|---|---------------|---|--------------|----------|
| 00CCHHHH     |   | ZERO      |   | ZERO     |   | ZERO          |   | ZERO         | ZEROZERO |

- `00` The double leading zero lexicographically sorts this special collection with the baked objects
- `CC` is the code of the special GUID format, `ec` is the code for root entity collection
- `HHHH` is the hive index with values between (inclusive) `1` and `4095`.
- `ZERO` always zero groups. There can be only one root entity collection per hive so the hive id is identifier enough.

## Local UID replacement
Uids are only available on backend connected dedicated servers. So for local workbench testing purposes a replacement is offered of the same GUID format and based on the order of connection of the main window and additional peer tools.

| bbbbdddd     | - | 0000      | - | 0000     | - | 0000          | - | 0000         | 00000001 |
|--------------|---|-----------|---|----------|---|---------------|---|--------------|----------|
| BOHEMUID     |   | ZERO      |   | ZERO     |   | ZERO          |   | UID1         | UID1UID1 |

- `BOHEMUID` Leading indicator `bbbbdddd` of a local replacement of the Bohemia player UID.
- `ZERO` always zero groups.
- `UID1` uid to a length of 12 based on the local connect [`playerId`](https://enfusionengine.com/api/redirect?to=enfusion://ScriptEditor/Scripts/Game/generated/GameMode/BaseGameMode.c;26).
