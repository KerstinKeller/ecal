# ========================= eCAL LICENSE =================================
# Copyright (C) 2016 - 2025 Continental Corporation
# Licensed under the Apache License, Version 2.0
# ========================= eCAL LICENSE =================================

import argparse
import os
import sys

import ecal.msg.proto.measurement as proto_measurement
from ecal.msg.common.measurement import MeasurementWriter

sys.path.insert(1, os.path.join(sys.path[0], '../core/pubsub/_protobuf'))
import person_pb2


def main() -> None:
    parser = argparse.ArgumentParser(description="Write protobuf person messages into an eCAL measurement")
    parser.add_argument("--output-dir", default="person_measurement", help="Measurement output directory")
    parser.add_argument("--file-name", default="measurement", help="Measurement base file name")
    args = parser.parse_args()

    with MeasurementWriter(args.output_dir, args.file_name, 500) as writer:
        channel = proto_measurement.create_channel(writer, "person", person_pb2.Person)

        for index, name in enumerate(["Alice", "Bob", "Charlie"]):
            person = person_pb2.Person()
            person.id = index
            person.name = name
            person.email = f"{name.lower()}@example.com"
            person.stype = person_pb2.Person.FEMALE if index % 2 == 0 else person_pb2.Person.MALE
            timestamp = 1_000_000 + index * 100_000
            channel.write(person, rcv_timestamp=timestamp, snd_timestamp=timestamp)
            print(f"wrote person id={person.id} name={person.name}")


if __name__ == "__main__":
    main()
