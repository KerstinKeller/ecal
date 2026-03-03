# ========================= eCAL LICENSE =================================
# Copyright (C) 2016 - 2025 Continental Corporation
# Licensed under the Apache License, Version 2.0
# ========================= eCAL LICENSE =================================

import argparse

import ecal.msg.string.measurement as string_measurement
from ecal.msg.common.measurement import MeasurementReader


def main() -> None:
    parser = argparse.ArgumentParser(description="Read string entries from an eCAL measurement")
    parser.add_argument("--input", default="string_measurement", help="Measurement path")
    args = parser.parse_args()

    with MeasurementReader(args.input) as reader:
        channel = string_measurement.open_channel(reader, "hello")
        print(f"channel metadata: {channel.metadata}")
        for entry in channel:
            print(f"rcv={entry.receive_timestamp} snd={entry.send_timestamp} text={entry.message}")


if __name__ == "__main__":
    main()
