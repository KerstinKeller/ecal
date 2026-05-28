# ========================= eCAL LICENSE =================================
# Copyright (C) 2016 - 2025 Continental Corporation
# Licensed under the Apache License, Version 2.0
# ========================= eCAL LICENSE =================================

import argparse
import os
import sys

import ecal.msg.proto.measurement as proto_measurement
from ecal.msg.common.measurement import MeasurementReader

sys.path.insert(1, os.path.join(sys.path[0], '../core/pubsub/_protobuf'))
import person_pb2


def main() -> None:
    parser = argparse.ArgumentParser(description="Read protobuf person messages from an eCAL measurement")
    parser.add_argument("--input", default="person_measurement", help="Measurement path")
    args = parser.parse_args()

    with MeasurementReader(args.input) as reader:
        channel = proto_measurement.open_channel(reader, "person", person_pb2.Person)
        print(f"channel metadata: {channel.metadata}")
        for entry in channel:
            print(
                f"rcv={entry.receive_timestamp} snd={entry.send_timestamp} "
                f"person_id={entry.message.id} name={entry.message.name}"
            )


if __name__ == "__main__":
    main()
