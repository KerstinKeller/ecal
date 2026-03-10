# ========================= eCAL LICENSE =================================
#
# Copyright (C) 2016 - 2025 Continental Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ========================= eCAL LICENSE =================================

"""Binary measurement2 API based on the low-level hdf5 wrapper."""

from dataclasses import dataclass
from typing import Iterator, NamedTuple

import ecal.measurement.hdf5 as ecal_hdf5


@dataclass(frozen=True)
class DataTypeInformation:
    name: str = ""
    encoding: str = ""
    descriptor: bytes = b""


@dataclass(frozen=True)
class ChannelMetadata:
    topic_name: str
    data_type: DataTypeInformation


class BinaryEntry(NamedTuple):
    rcv_timestamp: int
    snd_timestamp: int
    msg: bytes


def split_channel_type(channel_type: str) -> tuple[str, str]:
    if ":" in channel_type:
        return channel_type.split(":", 1)
    return "", channel_type


def combine_encoding_and_type(data_type: DataTypeInformation) -> str:
    if data_type.encoding:
        return f"{data_type.encoding}:{data_type.name}"
    return data_type.name


class BinaryChannelReader:
    def __init__(self, measurement_reader: "MeasurementReader", channel_name: str):
        self._measurement_reader = measurement_reader
        self._channel_name = channel_name
        self._entries = measurement_reader._reader.get_entries_info(channel_name)

    @property
    def metadata(self) -> ChannelMetadata:
        return self._measurement_reader.get_channel_metadata(self._channel_name)

    def __iter__(self) -> Iterator[BinaryEntry]:
        for position in range(len(self._entries)):
            yield self[position]

    def __len__(self) -> int:
        return len(self._entries)

    def __getitem__(self, entry_position: int) -> BinaryEntry:
        entry = self._entries[entry_position]
        data = self._measurement_reader._reader.get_entry_data(entry["id"])
        return BinaryEntry(
            rcv_timestamp=entry["rcv_timestamp"],
            snd_timestamp=entry["snd_timestamp"],
            msg=data,
        )


class MeasurementReader:
    def __init__(self, path: str):
        self._reader = ecal_hdf5.Meas(path)

    @property
    def channel_names(self):
        return self._reader.get_channel_names()

    def close(self) -> None:
        self._reader.close()

    def __enter__(self) -> "MeasurementReader":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        _ = exc_type, exc, tb
        self.close()

    def get_channel_metadata(self, channel_name: str) -> ChannelMetadata:
        type_encoding, type_name = split_channel_type(self._reader.get_channel_type(channel_name))
        return ChannelMetadata(
            topic_name=channel_name,
            data_type=DataTypeInformation(
                name=type_name,
                encoding=type_encoding,
                descriptor=self._reader.get_channel_description(channel_name),
            ),
        )

    def get_channel(self, channel_name: str) -> BinaryChannelReader:
        return BinaryChannelReader(self, channel_name)


class BinaryChannelWriter:
    def __init__(
        self,
        measurement_writer: "MeasurementWriter",
        channel_name: str,
        data_type: DataTypeInformation,
    ):
        self._measurement_writer = measurement_writer
        self._channel_name = channel_name
        self._data_type = data_type

        self._measurement_writer._writer.set_channel_description(channel_name, data_type.descriptor)
        self._measurement_writer._writer.set_channel_type(channel_name, combine_encoding_and_type(data_type))

    @property
    def data_type(self) -> DataTypeInformation:
        return self._data_type

    def write_entry(self, payload: bytes, rcv_timestamp: int, snd_timestamp: int) -> None:
        self._measurement_writer._writer.add_entry_to_file(payload, snd_timestamp, rcv_timestamp, self._channel_name)


class MeasurementWriter:
    def __init__(self, output_dir: str, file_name: str, max_size_per_file: int):
        self._writer = ecal_hdf5.Meas(output_dir, 1)
        self._writer.set_file_base_name(file_name)
        self._writer.set_max_size_per_file(max_size_per_file)

    def close(self) -> None:
        self._writer.close()

    def __enter__(self) -> "MeasurementWriter":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        _ = exc_type, exc, tb
        self.close()

    def create_channel(
        self,
        channel_name: str,
        data_type: DataTypeInformation = DataTypeInformation(),
    ) -> BinaryChannelWriter:
        return BinaryChannelWriter(self, channel_name, data_type)
