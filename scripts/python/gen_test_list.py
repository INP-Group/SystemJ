# -*- encoding: utf-8 -*-


def main():
    cnt_channels = 50000
    channels_per_worker = 5000

    cnt_workers = cnt_channels / channels_per_worker
    sql_add_lines = []
    sql_del_lines = []
    command_lines = []
    copy_lines = []

    for x in xrange(cnt_workers):
        name_worker = "worker_%s" % x
        command_lines.append("WORKER_ADD ||| %s\n" % name_worker)

    for x in xrange(0, cnt_channels):
        name_channel = 'test_channel_%s' % x
        sql_add_lines.append(
            "INSERT INTO channels (channel_name) VALUES ('%s'); \n" %
            name_channel)
        sql_del_lines.append('DELETE FROM channels WHERE channel_name = %s; \n'
                             % name_channel)
        command_lines.append(
            "CHL_ADD ||| %s ___ {'type': 'EasyFakeMonitor'} \n" % name_channel)
        copy_lines.append("%s\n" % name_channel)

    with open('copy_data.txt', 'w') as fio:
        fio.writelines(copy_lines)

    with open('channels_add.sql', 'w') as fio:
        fio.writelines(sql_add_lines)

    with open('channels_del.sql', 'w') as fio:
        fio.writelines(sql_del_lines)

    with open('commands.txt', 'w') as fio:
        fio.writelines(command_lines)


if __name__ == '__main__':
    main()
