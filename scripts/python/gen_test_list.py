# -*- encoding: utf-8 -*-



def main():


    sql_add_lines = []
    sql_del_lines = []
    command_lines = []
    for x in xrange(0, 1000):
        name_channel = 'test_channel_%s' % x
        sql_add = "INSERT INTO channels (channel_name) VALUES ('%s'); \n" % name_channel
        sql_del = "DELETE FROM channels WHERE channel_name = %s; \n" % name_channel
        command_line = "CHL_ADD ||| %s ___ {'type': 'EasyFakeMonitor'} \n" % name_channel

        sql_add_lines.append(sql_add)
        sql_del_lines.append(sql_del)
        command_lines.append(command_line)


    with open('channels_add.sql', 'w') as fio:
        fio.writelines(sql_add_lines)

    with open('channels_del.sql', 'w') as fio:
        fio.writelines(sql_del_lines)

    with open('commands.txt', 'w') as fio:
        fio.writelines(command_lines)
if __name__ == '__main__':
    main()