# Fuzzers

The fuzzers feed generated inputs to the code that reads what comes from
outside lpd8editor, and check properties that must hold whatever the input:

| Fuzzer | Code under test | Properties |
|---|---|---|
| `codec` | `lpd8::decodeProgram()`: device replies, program files, the settings' template | An error points inside the message. What is accepted is a complete SysEx message holding a valid program, which encodes back to the same bytes in the storage and 'send' formats. |
| `sysex_assembler` | `lpd8::SysexAssembler`: the bytes other MIDI clients send | Only complete messages (F0, data bytes, F7) within the size limit come out, the buffer never exceeds the limit, and cutting the input into pieces changes nothing. |
| `program_text` | `parseProgramText()`, the program file reader | A file that parses holds only ASCII digits and whitespace, with at most 3 digits per value, and 66 bytes, and writing them in the export format parses back to the same bytes. |
| `program_store` | `lpd8::fromJson()`, the programs file read at every start | What is accepted holds valid programs and names, and survives a round trip through `toJson()`. |
| `migration` | `lpd8::migrateSqliteDatabase()`, on old databases without CHECK constraints | The input chooses the values of the old tables (out of range, 64-bit, REAL, text, BLOB, NULL, missing rows). The migration succeeds, every program is migrated or reported, the file it writes loads, and a migrated program holds exactly the database's values, checked against an oracle that reads them with SQL's `typeof()`. |

All of them run under AddressSanitizer and UndefinedBehaviorSanitizer, so a
read one byte too far, an overflow or a use after free is a failure too.

## Running them

The seeds in `corpus/<fuzzer>/` run as ordinary tests (`fuzz_<fuzzer>`) in
every build. To fuzz, with Clang:

```bash
cmake --preset clang-fuzz
cmake --build --preset clang-fuzz
mkdir -p /tmp/codec-corpus && cp fuzz/corpus/codec/* /tmp/codec-corpus/
./build/clang-fuzz/fuzz/codec_fuzzer /tmp/codec-corpus -max_total_time=600
```

The fuzzer writes the inputs it finds interesting to the corpus folder (a
copy, so the seeds in the repository stay small), and an input that breaks a
property to `crash-<hash>` in the current folder. Replay one with any build:

```bash
./build/gcc-debug/fuzz/codec_replay crash-<hash>
```

The migration fuzzer writes a database and a programs file for every input,
so it runs a few hundred inputs a second rather than millions: give it more
time.

CI fuzzes each target for two minutes on every pull request (the `fuzz` job
of `.github/workflows/quality.yml`), and keeps the failing inputs as an
artifact.

## When a fuzzer fails

Fix the bug, then add the failing input to `corpus/<fuzzer>/` under a name
that says what it is: it becomes a regression test that runs on every build.

## Adding a fuzzer

Write `<name>_fuzzer.cpp` with `LLVMFuzzerInitialize()` and
`LLVMFuzzerTestOneInput()` (see `fuzzer.h`), add `<name>` to `_fuzzers` in
`CMakeLists.txt`, add it to the list of the `fuzz` job in
`.github/workflows/quality.yml`, and seed `corpus/<name>/` with valid inputs
and near misses.
