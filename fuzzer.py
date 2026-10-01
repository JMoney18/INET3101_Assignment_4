from pathlib import Path
import ctypes
import subprocess

SEAT_COUNT = 24
NAME_LENGTH = 50
HEADER = b"COLFLT01"


class Seat(ctypes.Structure):
    _fields_ = [
        ("seat_id", ctypes.c_int),
        ("assigned", ctypes.c_int),
        ("last_name", ctypes.c_char * NAME_LENGTH),
        ("first_name", ctypes.c_char * NAME_LENGTH),
    ]


def create_valid_data():
    records = (Seat * (SEAT_COUNT * 2))()

    for i in range(SEAT_COUNT * 2):
        records[i].seat_id = (i % SEAT_COUNT) + 1
        records[i].assigned = 0

    # Outbound seat 5.
    records[4].assigned = 1
    records[4].first_name = b"Mary Ann"
    records[4].last_name = b"Smith"

    # Inbound seat 7.
    records[SEAT_COUNT + 6].assigned = 1
    records[SEAT_COUNT + 6].first_name = b"Robin"
    records[SEAT_COUNT + 6].last_name = b"Lee"

    return HEADER + bytes(records)


def change_bytes(original, offset, replacement):
    changed = bytearray(original)
    changed[offset:offset + len(replacement)] = replacement
    return bytes(changed)

def run_tests(output_folder, cases):
    executable = output_folder.parent / "solution"

    if not executable.is_file():
        print("Compile solution.c before running the tests.")
        return

    expected_errors = {
        "empty_file": "Could not read the complete file header.",
        "short_header": "Could not read the complete file header.",
        "bad_header": "Invalid file header or unsupported version.",
        "truncated_outbound": "Incomplete Outbound flight data.",
        "truncated_inbound": "Incomplete Inbound flight data.",
        "oversized": "Unexpected extra bytes after flight records.",
        "invalid_seat_id": "invalid seat ID",
        "invalid_status": "invalid assignment status",
        "nonprintable_name": "invalid passenger name",
        "unterminated_name": "invalid passenger name",
        "blank_assigned_name": "invalid passenger name",
    }

    results = []

    for case_name in cases:
        case_folder = output_folder / case_name
        data_file = case_folder / "flight_data.bin"
        before = data_file.read_bytes()

        try:
            result = subprocess.run(
                [str(executable)],
                input="a\na\nc\nf\nb\na\nc\nf\nc\n",
                text=True,
                capture_output=True,
                cwd=case_folder,
                timeout=5,
            )
        except subprocess.TimeoutExpired:
            message = f"FAIL: {case_name} — exceeded 5 seconds"
            results.append(message)
            print(message)
            continue

        output = result.stdout + "\n--- Errors ---\n" + result.stderr

        (case_folder / "output.log").write_text(
            output, encoding="utf-8"
        )

        if case_name == "valid":
            passed = (
                result.returncode == 0
                and "Both flights loaded" in result.stdout
                and "Seat 5: Smith, Mary Ann" in result.stdout
                and "Seat 7: Lee, Robin" in result.stdout
                and result.stdout.count("Number of empty seats: 23") == 2
                and "Both flights saved" in result.stdout
            )
        else:
            unchanged = (
                data_file.is_file()
                and data_file.read_bytes() == before
            )

            passed = (
                result.returncode == 1
                and expected_errors[case_name] in result.stderr
                and "Startup stopped." in result.stderr
                and unchanged
            )

        status = "PASS" if passed else "FAIL"
        message = (
            f"{status}: {case_name} "
            f"(exit status {result.returncode})"
        )
        results.append(message)
        print(message)

    passed_count = sum(line.startswith("PASS:") for line in results)
    summary = f"\nPassed {passed_count} of {len(cases)} tests."
    print(summary)

    (output_folder.parent / "fuzz_results.log").write_text(
        "\n".join(results) + summary + "\n",
        encoding="utf-8",
    )

def main():
    output_folder = Path(__file__).resolve().parent / "fuzz_cases"
    output_folder.mkdir(exist_ok=True)

    valid = create_valid_data()
    seat_size = ctypes.sizeof(Seat)

    # Locate Outbound seat 5 inside the file.
    seat_offset = len(HEADER) + 4 * seat_size
    id_offset = seat_offset + Seat.seat_id.offset
    status_offset = seat_offset + Seat.assigned.offset
    name_offset = seat_offset + Seat.first_name.offset

    cases = {
        "valid": valid,
        "empty_file": b"",
        "short_header": HEADER[:4],
        "bad_header": b"BADFLT01" + valid[len(HEADER):],
        "truncated_outbound": valid[:len(HEADER) + 10 * seat_size],
        "truncated_inbound": valid[:-1],
        "oversized": valid + b"EXTRA_BYTES",
        "invalid_seat_id": change_bytes(
            valid, id_offset, bytes(ctypes.c_int(99))
        ),
        "invalid_status": change_bytes(
            valid, status_offset, bytes(ctypes.c_int(7))
        ),
        "nonprintable_name": change_bytes(
            valid, name_offset, b"\x01"
        ),
        "unterminated_name": change_bytes(
            valid, name_offset, b"A" * NAME_LENGTH
        ),
        "blank_assigned_name": change_bytes(
            valid, name_offset, b"\0" * NAME_LENGTH
        ),
    }

    for case_name, data in cases.items():
        case_folder = output_folder / case_name
        case_folder.mkdir(exist_ok=True)

        destination = case_folder / "flight_data.bin"
        destination.write_bytes(data)

        print(f"{case_name}: wrote {len(data)} bytes")

    print(f"\nSeat structure size: {seat_size} bytes")
    print(f"Expected valid file size: {len(valid)} bytes")
    print(f"Created {len(cases)} test cases in {output_folder}")
    run_tests(output_folder, cases)


if __name__ == "__main__":
    main()