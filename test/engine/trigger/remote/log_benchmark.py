import argparse
import logging
import random
import time

from logging.handlers import TimedRotatingFileHandler
from datetime import datetime
from faker import Faker
from faker.providers import internet, user_agent, lorem, misc
def nginx(args, logger, faker):
    exampleLog = f'{faker.ipv4()} - - [{datetime.now().strftime('%d/%b/%Y:%H:%M:%S %z')}] "{faker.http_method()} {faker.url()} HTTP/1.1" {faker.status_code()} {random.randint(1, 10000)} "{faker.url()}" "{faker.user_agent()}"'
    sleepInterval = len(exampleLog) / args.rate * 1024 * 1024
    startTime = datetime.now()
    while True:
        now = datetime.now()
        logger.info(f'{faker.ipv4()} - - [{now.strftime('%d/%b/%Y:%H:%M:%S %z')}] "{faker.http_method()} {faker.url()} HTTP/1.1" {faker.status_code()} {random.randint(1, 10000)} "{faker.url()}" "{faker.user_agent()}"')
        time.sleep(sleepInterval)
        if (now - startTime).seconds > args.duration:
            break
def parse_custom_arg_to_dict(custom_arg):
    custom_arg_dict = {}
    for arg in custom_arg:
        key, value = arg.split('=')
        custom_arg_dict[key] = value
    return custom_arg_dict

def main():
    parser = argparse.ArgumentParser(description='Log Generator Arg Parser')
    parser.add_argument('--mode', type=str, default='nginx', help='Log Type')
    parser.add_argument('--path', type=str, default='default.log', help='Log Path')
    parser.add_argument('--rate', type=int, default=10, help='Log Generate Rate (MB/s)')
    parser.add_argument('--duration', type=int, default=60, help='Log Generate Duration (s)')
    parser.add_argument('--custom', nargs='*', type=parse_custom_arg_to_dict, help='Custom Args, in the format of key=value')

    args = parser.parse_args()

    logger = logging.getLogger('log_generator')
    logger.setLevel(logging.INFO)
    # 快速轮转来模拟比较极端的情况
    handler = TimedRotatingFileHandler(args.path, when="s", interval=5, backupCount=3)
    formatter = logging.Formatter('%(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    # 随机生成器
    faker = Faker(['en_US', 'zh_CN'])
    faker.add_provider(internet)
    faker.add_provider(user_agent)
    faker.add_provider(lorem)
    faker.add_provider(misc)

    # 生成数据
    if args.mode == 'nginx':
        nginx(args, logger, faker)


if __name__ == '__main__':
    main()
