import sys, logging

class ColoredFormatterSpecification:
    RESET_SEQUENCE = "\033[0m"
    COLOR_SEQUENCE = "\033[1;%dm"
    BOLD_SEQUENCE = "\033[1m"

    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(8)

    COLOR_SCHEME = {
        logging.DEBUG: BLUE,
        logging.INFO: GREEN,
        logging.WARNING: YELLOW,
        logging.ERROR: RED,
        logging.CRITICAL: MAGENTA
    }

    FORMAT_STR = "[%(asctime)s] [%(name)s] [%(levelname)s]: %(message)s"

class ColoredFormatter(logging.Formatter):
    __FormatDictionary = {loglevelno:''.join([ColoredFormatterSpecification.COLOR_SEQUENCE % (30 + ColoredFormatterSpecification.COLOR_SCHEME[loglevelno]), ColoredFormatterSpecification.FORMAT_STR, ColoredFormatterSpecification.RESET_SEQUENCE]) 
            for loglevelno in (logging.DEBUG, logging.INFO, logging.WARNING, logging.ERROR, logging.CRITICAL)}
    __DateFMT = "%H:%M:%S"

    def format(self, record):
        log_fmt = ColoredFormatter.__FormatDictionary.get(record.levelno)
        formatter = logging.Formatter(log_fmt, datefmt=ColoredFormatter.__DateFMT)
        return formatter.format(record)

class ColoredLogger:
    Logger = logging.getLogger("FLAMEBERRY")

    def InitLogger() -> None:
        ColoredLogger.Logger.setLevel(logging.DEBUG)
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(logging.DEBUG)
        ch.setFormatter(ColoredFormatter())
        ColoredLogger.Logger.addHandler(ch)
