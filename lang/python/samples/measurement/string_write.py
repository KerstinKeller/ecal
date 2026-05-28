# ========================= eCAL LICENSE =================================
# Copyright (C) 2016 - 2025 Continental Corporation
# Licensed under the Apache License, Version 2.0
# ========================= eCAL LICENSE =================================

import argparse

import ecal.msg.string.measurement as string_measurement
from ecal.msg.common.measurement import MeasurementWriter


def main() -> None:
    parser = argparse.ArgumentParser(description="Write string entries into an eCAL measurement")
    parser.add_argument("--output-dir", default="string_measurement", help="Measurement output directory")
    parser.add_argument("--file-name", default="measurement", help="Measurement base file name")
    args = parser.parse_args()

    with MeasurementWriter(args.output_dir, args.file_name, 500) as writer:
        channel = string_measurement.create_channel(writer, "hello")
        for index, text in enumerate(["hello", "from", "ecal", "measurement"]):
            timestamp = 1_000_000 + index * 100_000
            channel.write(text, rcv_timestamp=timestamp, snd_timestamp=timestamp)
            print(f"wrote text={text}")


if __name__ == "__main__":
    main()
