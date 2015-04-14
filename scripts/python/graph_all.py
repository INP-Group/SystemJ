# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import datetime
import os

import matplotlib.pyplot as plt

from project.settings import POSTGRESQL_DB, MEDIA_FOLDER, check_and_create
from project.settings import POSTGRESQL_HOST
from project.settings import POSTGRESQL_PASSWORD
from project.settings import POSTGRESQL_TABLE
from project.settings import POSTGRESQL_USER
from src.storage.postgresql import PostgresqlStorage


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
        data_of_channels[x[0]].append((x[1], x[2]))

    return data_of_channels


def show_plot(data, name):
    x_val = data[0]
    y_val = data[1]
    # plt.hold(False)
    plt.title(name)
    plt.xlabel('Time')
    plt.ylabel('Values')
    plt.plot(x_val, y_val)
    plt.plot(x_val, y_val, 'or')
    plt.savefig(os.path.join(MEDIA_FOLDER, 'plots', '%s.png' % name))
    plt.clf()
    plt.cla()
    # plt.show()


def prepare_data(data):
    times = []
    values = []
    prev_time = data[0][0] if isinstance(data[0][0], datetime.datetime) else None
    prev_value = data[0][1] if isinstance(data[0][1], (float, int)) else None

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
            prev_time = cur_time
            prev_value = cur_value
        times.append(cur_time)
        values.append(cur_value)

    return times, values


def main():
    channels = []
    for x in xrange(0, 99):
        channels.append('linvac.vacmatrix.Imes%s' % x)

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
                info_for_plot = prepare_data(value)

                name = '%s_%s_%s' % (key, time_start, time_end)
                show_plot(info_for_plot, name)
        except (TypeError, IndexError) as e:
            print(key, e, value)
            raise
        # else:
        #     break


if __name__ == '__main__':
    main()
