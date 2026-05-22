# Monitor Polish Validation

`monitor_polish_runner.c` checks the user-facing monitor improvements:

- the expanded banner includes CPU, ROM, and cartridge map lines
- `?` prints readable command help
- `Mhhhh` dumps one 16-byte line
- `Mhhhh.kkkk` dumps a WOZMON-style address range
- `N` dumps the next 16-byte line
- `Whhhhbb` prints `hhhh=bb OK`

Run it with:

```sh
programs/tests/run_monitor_polish.sh
```
