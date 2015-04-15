# -*- encoding: utf-8 -*-
"""Модуль с функциями логгирования."""
import logging
import os
from project.settings import LOG_FOLDER, LOG

LOG_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'


def __get_handler(filename, level, log_format):
    handler = logging.FileHandler(os.path.join(LOG_FOLDER, filename))
    handler.setLevel(level)
    handler.setFormatter(logging.Formatter(log_format))
    return handler


def __get_handler_std(level, log_format):
    handler = logging.StreamHandler()
    handler.setLevel(level)
    handler.setFormatter(logging.Formatter(log_format))
    return handler


def __init_logger(level, file, log_format=None):
    if log_format is None:
        global LOG_FORMAT
        log_format = LOG_FORMAT
    log_obj = logging.getLogger()
    log_obj.setLevel(logging.DEBUG)
    log_obj.addHandler(__get_handler(file, level, log_format))
    # ---
    # для принтов в консоль
    if LOG and level == logging.DEBUG:
        log_obj.addHandler(__get_handler_std(level, log_format))
    # ---
    return log_obj


def __get_log_func(level, log_obj):
    return {
        logging.INFO: log_obj.info,
        logging.DEBUG: log_obj.debug,
        logging.ERROR: log_obj.error,
        logging.CRITICAL: log_obj.critical,
    }.get(level)


def __log_method(level, file):
    log_object = __init_logger(level, file)
    log_func = __get_log_func(level, log_object)
    assert log_func is not None

    def _level_info(*args, **kwargs):
        message = str(*args) + str(**kwargs)
        log_object.setLevel(level)
        log_func(message)

    return _level_info


log_info = __log_method(logging.INFO, 'info.log')
log_debug = __log_method(logging.DEBUG, 'debug.log')
log_critical = __log_method(logging.CRITICAL, 'critical.log')
log_error = __log_method(logging.ERROR, 'error.log')
log_track = __log_method(logging.INFO, 'tracking.log')

__all__ = [
    'log_info',
    'log_debug',
    'log_critical',
    'log_error',
    'log_track',
]
