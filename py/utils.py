"""Utilities copied from https://github.com/cvangysel/cvangysel-common."""

import argparse
import collections
import itertools
import io
import logging
import os
import shutil
import socket
import tempfile
import sys


def get_hostname():
    return socket.gethostbyaddr(socket.gethostname())[0]


def get_formatter():
    return logging.Formatter(
        '%(asctime)s [%(threadName)s.{}] '
        '[%(name)s] [%(levelname)s]  '
        '%(message)s'.format(get_hostname()))


def configure_logging(args, output_path=None):
    loglevel = args.loglevel if hasattr(args, 'loglevel') else 'INFO'

    # Set logging level.
    numeric_log_level = getattr(logging, loglevel.upper(), None)
    if not isinstance(numeric_log_level, int):
        raise ValueError('Invalid log level: %s' % loglevel.upper())

    # Configure logging level.
    logging.basicConfig(level=numeric_log_level)
    logging.getLogger().setLevel(numeric_log_level)

    # Set-up log formatting.
    log_formatter = get_formatter()

    for handler in logging.getLogger().handlers:
        handler.setFormatter(log_formatter)

    logging.info('Arguments: %s', args)


def positive_int(value):
    try:
        value = int(value)
        assert value >= 0

        return value
    except:
        raise argparse.ArgumentTypeError(
            '"{0}" is not a valid positive int'.format(value))


def existing_file_path(value):
    value = str(value)

    if os.path.exists(value):
        return value
    else:
        raise argparse.ArgumentTypeError(
            'File "{0}" does not exists.'.format(value))


def nonexisting_file_path(value):
    value = str(value)

    if not os.path.exists(value):
        return value
    else:
        raise argparse.ArgumentTypeError(
            'File "{0}" already exists.'.format(value))


def existing_directory_path(value):
    value = str(value)

    if os.path.isdir(value):
        return value
    else:
        raise argparse.ArgumentTypeError(
            '"{0}" is not a directory'.format(value))


def write_ranking(model_name, data, out_f,
                  max_objects_per_query,
                  skip_sorting,
                  format):
    """
    Write a run to an output file.

    Parameters:
        - model_name: identifier of run.
        - data: dictionary mapping query_id to object_assesments;
            object_assesments is an iterable (list or tuple) of
            (relevance, object_id) pairs.

            The object_assesments iterable is sorted by decreasing order.
        - out_f: output file stream.
        - max_objects_per_query: cut-off for number of objects per query.
    """
    for subject_id, object_assesments in data.items():
        if not object_assesments:
            logging.warning('Received empty ranking for %s; ignoring.',
                            subject_id)

            continue

        # Probe types, to make sure everything goes alright.
        # assert isinstance(object_assesments[0][0], float) or \
        #     isinstance(object_assesments[0][0], np.float32)
        assert isinstance(object_assesments[0][1], str) or \
            isinstance(object_assesments[0][1], bytes)

        if not skip_sorting:
            object_assesments = sorted(object_assesments, reverse=True)

        if max_objects_per_query < sys.maxsize:
            object_assesments = object_assesments[:max_objects_per_query]

        if isinstance(subject_id, bytes):
            subject_id = subject_id.decode('utf8')

        for rank, (relevance, object_id) in enumerate(object_assesments):
            if isinstance(object_id, bytes):
                object_id = object_id.decode('utf8')

            out_f.write(format.format(
                subject=subject_id,
                object=object_id,
                rank=rank + 1,
                relevance=relevance,
                model_name=model_name))


def write_run(model_name, data, out_f,
              max_objects_per_query=sys.maxsize,
              skip_sorting=False):
    return write_ranking(
        model_name, data, out_f, max_objects_per_query, skip_sorting,
        '{subject} Q0 {object} {rank} {relevance:.40f} {model_name}\n')


class TRECRunWriter(object):

    def __init__(self, name, rank_cutoff=sys.maxsize, write_fn=write_run):
        self.name = name
        self.rank_cutoff = rank_cutoff

        self.write_fn = write_fn

        self.tmp_file = tempfile.NamedTemporaryFile(
            mode='w', delete=False)

        logging.info('Writing temporary run to %s.',
                     self.tmp_file.name)

    def add_ranking(self, subject_id, object_assesments):
        assert self.tmp_file

        self.write_fn(self.name,
                      data={subject_id: object_assesments},
                      out_f=self.tmp_file,
                      max_objects_per_query=self.rank_cutoff)

    def close_and_return_temporary_path(self):
        assert self.tmp_file

        tmp_file_path = self.tmp_file.name

        self.tmp_file.close()

        return tmp_file_path

    def close_and_write(self, out_path, overwrite=True):
        if not overwrite:
            assert not os.path.exists(out_path)
        elif os.path.exists(out_path):
            logging.warning('Overwriting run %s.', out_path)

        tmp_file_path = self.close_and_return_temporary_path()
        shutil.copy(tmp_file_path, out_path)

        os.remove(tmp_file_path)


def read_queries(file_or_files,
                 max_queries=sys.maxsize, delimiter=';'):
    assert max_queries >= 0 or max_queries is None

    queries = collections.OrderedDict()

    if not isinstance(file_or_files, list) and \
            not isinstance(file_or_files, tuple):
        if not isinstance(file_or_files, io.IOBase):
            file_or_files = list(file_or_files)
        else:
            file_or_files = [file_or_files]

    for f in file_or_files:
        assert isinstance(f, io.IOBase), type(f)

        for line in f:
            assert(isinstance(line, str))

            line = line.strip()

            if not line:
                continue

            try:
                query_id, terms = line.split(delimiter, 1)
            except ValueError:
                logging.warning('Unable to process "%s" in queries list.',
                                line)

                continue

            if query_id in queries and (queries[query_id] != terms):
                logging.error('Duplicate query "%s" (%s vs. %s).',
                              query_id, queries[query_id], terms)

            queries[query_id] = terms

            if max_queries > 0 and len(queries) >= max_queries:
                break

    return queries


def parse_queries(index, dictionary, query_path,
                  strict=False, num_queries=None):
    query_f = list(map(lambda filename: open(filename, 'r'), [query_path]))
    queries = read_queries(query_f)
    [q_.close() for q_ in query_f]

    if num_queries:
        queries = itertools.islice(queries.items(), num_queries)
    else:
        queries = queries.items()

    for idx, (query_id, query_text) in enumerate(queries):
        try:
            query_tokens = index.tokenize(query_text)
        except OSError:
            raise RuntimeError(
                'Unable to tokenize query {} with text "{}".'.format(
                    query_id, query_text))

        query_token_ids = [dictionary.translate_token(token)
                           for token in query_tokens]

        logging.info('Query %s: "%s" -> %s (%s)',
                     query_id, query_text,
                     query_tokens, query_token_ids)

        if all(query_token_id is None
               for query_token_id in query_token_ids):
            logging.warning('Skipping query %s as all tokens are OoV.',
                            query_id)

            query_token_ids = None
        if strict and any(query_token_id is None
                          for query_token_id in query_token_ids):
            logging.warning('Skipping query %s as '
                            'at least one token is OoV.',
                            query_id)

            query_token_ids = None

        yield query_id, query_token_ids
