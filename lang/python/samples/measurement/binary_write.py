# ========================= eCAL LICENSE =================================
# Copyright (C) 2016 - 2025 Continental Corporation
# Licensed under the Apache License, Version 2.0
# ========================= eCAL LICENSE =================================

import argparse

from ecal.measurement2.measurement import DataTypeInformation, MeasurementWriter


def main() -> None:
    parser = argparse.ArgumentParser(description="Write binary entries into an eCAL measurement")
    parser.add_argument("--output-dir", default="binary_measurement", help="Measurement output directory")
    parser.add_argument("--file-name", default="measurement", help="Measurement base file name")
    args = parser.parse_args()

    with MeasurementWriter(args.output_dir, args.file_name, 500) as writer:
        blob_type = DataTypeInformation(name="blob.v1", encoding="binary")
        channel = writer.create_channel(channel_name="blob", data_type=blob_type)
        for index in range(5):
            payload = bytes([index, index + 1, index + 2, 255 - index])
            timestamp = 1_000_000 + index * 100_000
            channel.write_entry(payload, rcv_timestamp=timestamp, snd_timestamp=timestamp)
            print(f"wrote index={index} payload={payload.hex()}")


if __name__ == "__main__":
    main()
