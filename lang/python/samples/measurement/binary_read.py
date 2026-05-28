# ========================= eCAL LICENSE =================================
# Copyright (C) 2016 - 2025 Continental Corporation
# Licensed under the Apache License, Version 2.0
# ========================= eCAL LICENSE =================================

import argparse

from ecal.measurement2.measurement import MeasurementReader


def main() -> None:
    parser = argparse.ArgumentParser(description="Read binary entries from an eCAL measurement")
    parser.add_argument("--input", default="binary_measurement", help="Measurement path")
    args = parser.parse_args()

    with MeasurementReader(args.input) as reader:
        channel = reader.get_channel("blob")
        print(f"channel metadata: {channel.metadata}")
        for entry in channel:
            print(
                f"rcv={entry.rcv_timestamp} snd={entry.snd_timestamp} size={len(entry.msg)} "
                f"payload={entry.msg.hex()}"
            )


if __name__ == "__main__":
    main()
