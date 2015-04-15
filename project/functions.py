# -*- encoding: utf-8 -*-
"""Файл соддержит общие функции для всего проекта."""
import os


def check_and_create(folder):
    """Проверяет существование папки и если ее нет, то создает.

    :param folder:
    :return:

    """
    if not os.path.exists(folder):
        os.makedirs(folder)
