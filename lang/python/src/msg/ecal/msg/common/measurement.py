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

import typing

from .serializer import Deserializer, Serializer
import ecal.measurement2.measurement as measurement_core

T = typing.TypeVar("T")

DataTypeInformation = measurement_core.DataTypeInformation


class ReceiveEntry(typing.Generic[T], typing.NamedTuple):
    send_timestamp: int
    receive_timestamp: int
    message: T


class ChannelReader(typing.Generic[T]):
    def __init__(self, binary_channel: measurement_core.BinaryChannelReader, deserializer: Deserializer[T]) -> None:
        self._binary_channel = binary_channel
        self._deserializer = deserializer

    @property
    def metadata(self) -> measurement_core.ChannelMetadata:
        return self._binary_channel.metadata

    def __len__(self) -> int:
        return len(self._binary_channel)

    def __iter__(self) -> typing.Iterator[ReceiveEntry[T]]:
        for idx in range(len(self)):
            yield self[idx]

    def __getitem__(self, index: int) -> ReceiveEntry[T]:
        binary_entry = self._binary_channel[index]
        data_type = self.metadata.data_type

        if not self._deserializer.accepts_data_with_type(data_type):
            raise TypeError(
                f"Deserializer does not accept channel '{self.metadata.topic_name}' "
                f"with encoding '{data_type.encoding}' and name '{data_type.name}'."
            )

        return ReceiveEntry(
            send_timestamp=binary_entry.snd_timestamp,
            receive_timestamp=binary_entry.rcv_timestamp,
            message=self._deserializer.deserialize(binary_entry.msg, data_type),
        )


class ChannelWriter(typing.Generic[T]):
    def __init__(self, binary_channel: measurement_core.BinaryChannelWriter, serializer: Serializer[T]) -> None:
        self._binary_channel = binary_channel
        self._serializer = serializer

    def write(self, message: T, rcv_timestamp: int, snd_timestamp: int) -> None:
        self._binary_channel.write_entry(self._serializer.serialize(message), rcv_timestamp, snd_timestamp)


class MeasurementReader:
    def __init__(self, path: str):
        self._reader = measurement_core.MeasurementReader(path)

    @property
    def channel_names(self):
        return self._reader.channel_names

    def close(self) -> None:
        self._reader.close()

    def __enter__(self) -> "MeasurementReader":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        _ = exc_type, exc, tb
        self.close()

    def open_binary_channel(self, channel_name: str) -> measurement_core.BinaryChannelReader:
        return self._reader.get_channel(channel_name)

    def create_channel(self, channel_name: str, deserializer: Deserializer[T]) -> ChannelReader[T]:
        return ChannelReader(self._reader.get_channel(channel_name), deserializer)


class MeasurementWriter:
    def __init__(self, output_dir: str, file_name: str, max_size_per_file: int):
        self._writer = measurement_core.MeasurementWriter(output_dir, file_name, max_size_per_file)

    def close(self) -> None:
        self._writer.close()

    def __enter__(self) -> "MeasurementWriter":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        _ = exc_type, exc, tb
        self.close()

    def create_binary_channel(
        self,
        channel_name: str,
        data_type: DataTypeInformation = DataTypeInformation(),
    ) -> measurement_core.BinaryChannelWriter:
        return self._writer.create_channel(channel_name=channel_name, data_type=data_type)

    def create_channel(self, channel_name: str, serializer: Serializer[T]) -> ChannelWriter[T]:
        binary_channel = self.create_binary_channel(channel_name, serializer.get_data_type_information())
        return ChannelWriter(binary_channel, serializer)
