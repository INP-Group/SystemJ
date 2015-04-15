# -*- encoding: utf-8 -*-
import os

from project.settings import BIN_FOLDER
from project.settings import DEPLOY_FOLDER
from project.settings import LIST_SERVICE
from project.settings import LOG_FOLDER
from project.settings import PROJECT_DIR
from project.settings import USER
from project.settings import VENV_PYTHON
from project.settings import log_info


def create_supervisord_service(service):
    log_folder = os.path.join(LOG_FOLDER, 'services')
    service_info = {
        'service': service,
        'log_file_error': os.path.join(log_folder, '%s_error.log' % service),
        'log_file_out': os.path.join(log_folder, '%s_out.log' % service),
        'project_dir': PROJECT_DIR,
        'user': USER,
        'venv': VENV_PYTHON,
        'service_path': os.path.join(BIN_FOLDER, '%s.py' % service)
    }

    template = """[program:service_{service}]
command={venv} {service_path}
directory={project_dir}
user={user}
autostart=true
autorestart=true
redirect_stderr=true
stderr_logfile={log_file_error}
stdout_logfile={log_file_out}
""".format(**service_info)

    conf_path = os.path.join(DEPLOY_FOLDER, 'supervisord',
                             'supervisord_%s.conf' % service)

    with open(conf_path, 'w') as fio:
        fio.write(template)

    supervisord_folder = '/etc/supervisor/conf.d/'
    log_info('sudo ln -s %s %s' % (conf_path, supervisord_folder))


for service in LIST_SERVICE:
    create_supervisord_service(service)
