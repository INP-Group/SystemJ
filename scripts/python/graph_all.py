# -*- encoding: utf-8 -*-
from src.utils.kvstorage import load

try:
    from __init__ import *
except ImportError:
    pass

import datetime
import os

from project.logs import log_error
from project.settings import MEDIA_FOLDER
from project.settings import POSTGRESQL_DB
from project.settings import POSTGRESQL_HOST
from project.settings import POSTGRESQL_PASSWORD
from project.settings import POSTGRESQL_TABLE
from project.settings import POSTGRESQL_USER
from project.settings import check_and_create
from src.storage.postgresql import PostgresqlStorage
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


def get_data_from_storage(channels, time_start, time_end):
    storage = PostgresqlStorage(database=POSTGRESQL_DB,
                                user=POSTGRESQL_USER,
                                password=POSTGRESQL_PASSWORD,
                                tablename=POSTGRESQL_TABLE,
                                host=POSTGRESQL_HOST)

    format = '%Y-%m-%d %H:%M:%S'
    time_start = time_start.strftime(format)
    time_end = time_end.strftime(format)
    data = storage.get_data(channels=channels, time_start=time_start,
                            time_end=time_end)

    data_of_channels = {}

    for x in channels:
        data_of_channels[x] = []

    for x in data:
        dtime = x[0]
        # хак
        # чтобы время соответствовало формату '%Y-%m-%d %H:%M:%S.%f'
        if not dtime.microsecond:
            dtime = dtime.replace(microsecond=1)
        data_of_channels[x[-1]].append((dtime, x[1]))  # data, value, channel_id

    return data_of_channels


def show_plot(times, values, np_test, name):
    x_val, y_val = np.loadtxt(np_test, delimiter=',', unpack=True,
                              converters={
                                  0: mdates.strpdate2num(
                                      '%Y-%m-%d %H:%M:%S.%f')})

    # x_val = times
    # y_val = values
    # plt.hold(False)
    plt.title(name)
    plt.xlabel('Time')
    plt.ylabel('Values')
    plt.plot_date(x=x_val,
                  y=y_val,
                  marker='o',
                  markerfacecolor='red',
                  fmt='b-',
                  label='value',
                  linewidth=2
                  )
    # plt.plot(x_val, y_val)
    # plt.plot(x_val, y_val, 'or')
    plt.savefig(os.path.join(MEDIA_FOLDER, 'plots', '%s.png' % name))
    plt.clf()
    plt.cla()
    # plt.show()


def prepare_data(data):
    times = []
    values = []
    prev_time = data[0][0] if isinstance(
        data[0][0],
        datetime.datetime) else None
    prev_value = data[0][1] if isinstance(data[0][1], (float, int)) else None
    np_test = []

    assert prev_value is not None
    assert prev_time is not None

    for x in data:
        cur_time = x[0] if isinstance(x[0], datetime.datetime) else x[1]
        cur_value = x[0] if isinstance(x[1], datetime.datetime) else x[1]
        if prev_time < cur_time:
            delta = cur_time - prev_time
            for t in xrange(1, int(delta.total_seconds())):
                times.append(prev_time + datetime.timedelta(seconds=t))
                values.append(prev_value)
                np_test.append("%s,%s" % (
                    prev_time + datetime.timedelta(seconds=t), prev_value))
            prev_time = cur_time
            prev_value = cur_value
        times.append(cur_time)
        values.append(cur_value)
        np_test.append("%s,%s" % (cur_time, cur_value))

    return times, values, np_test


def main():
    kvstorage = load()
    channels = []
    for x in xrange(0, 10):
        # channels.append('linvac.vacmatrix.Imes%s' % x)
        channels.append(kvstorage.get('linvac.vacmatrix.Imes%s' % x))

    time_start = datetime.datetime(year=2015,
                                   month=4,
                                   day=1,
                                   hour=0,
                                   minute=0,
                                   second=0,
                                   microsecond=0)

    time_end = datetime.datetime(year=2015,
                                 month=4,
                                 day=3,
                                 hour=0,
                                 minute=0,
                                 second=0,
                                 microsecond=0)

    data_of_channels = get_data_from_storage(channels, time_start, time_end)
    check_and_create(os.path.join(MEDIA_FOLDER, 'plots'))

    for key, value in data_of_channels.items():
        try:
            if value:
                times, values, np_test = prepare_data(value)

                name = '%s_%s_%s' % (
                kvstorage.get_name_by_id(key), time_start, time_end)
                show_plot(times, values, np_test, name)
        except (TypeError, IndexError) as e:
            log_error(key, e, value)
            raise
        # else:
        #     break


if __name__ == '__main__':
    main()
