# Test fixtures

- `exported-program.lpd8`: a program file written by the SQLite version's
  export (commit 3a994a4), byte for byte, so that the export from the JSON
  store can be checked against it (`programfile_test`). The program is the
  factory program with channel 4 (stored as 3), pad 1's note at 60 and its
  toggle on, and knob 8's high value at 100. Like every exported file, it
  ends with a space and no newline: `.gitattributes` keeps
  `git diff --check` from flagging that space, and the file must not be
  "fixed" by an editor.
