# Notes

- insertRecord uses values from QSqlRecord to update the model, not what was created by the database (including its own default values)

- Project creation:
    1. When QAction triggered: default values (from default sysex ?)
    2. When imported from file: sysex from file
    3. When read from hardware: sysex from MIDI
    4. When undo delete: from sysex serialization ?

- Serialization to sysex:
    1. When sending to file
    2. When sending to MIDI
    3. When storing in UndoStack

-
