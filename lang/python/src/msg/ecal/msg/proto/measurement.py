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

from ..common import measurement as common_measurement
from .serializer import DynamicSerializer, Serializer

T = typing.TypeVar("T")


def create_channel(
    measurement_writer: common_measurement.MeasurementWriter,
    channel_name: str,
    protobuf_message_type: typing.Type[T],
) -> common_measurement.ChannelWriter[T]:
    serializer = Serializer(protobuf_message_type, common_measurement.DataTypeInformation)
    return measurement_writer.create_channel(channel_name, serializer)


def open_channel(
    measurement_reader: common_measurement.MeasurementReader,
    channel_name: str,
    protobuf_message_type: typing.Type[T],
) -> common_measurement.ChannelReader[T]:
    deserializer = Serializer(protobuf_message_type, common_measurement.DataTypeInformation)
    return measurement_reader.create_channel(channel_name, deserializer)


def open_dynamic_channel(
    measurement_reader: common_measurement.MeasurementReader,
    channel_name: str,
) -> common_measurement.ChannelReader[typing.Any]:
    deserializer = DynamicSerializer(common_measurement.DataTypeInformation)
    return measurement_reader.create_channel(channel_name, deserializer)
