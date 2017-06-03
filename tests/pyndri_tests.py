import gc
import operator
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

import pyndri


class PyndriTest(unittest.TestCase):

    def setUp(self):
        if hasattr(sys, 'gettotalrefcount'):
            gc.collect()
            self.ref_count_before_test = sys.gettotalrefcount()

    def tearDown(self):
        # For debugging purposes.
        if hasattr(sys, 'gettotalrefcount'):
            gc.collect()
            print(self._testMethodName,
                  sys.gettotalrefcount() - self.ref_count_before_test)


class ParsingTest(PyndriTest):

    def test_stemming(self):
        self.assertEqual(pyndri.stem('predictions'), 'prediction')
        self.assertEqual(pyndri.stem('marketing'), 'marketing')
        self.assertEqual(pyndri.stem('strategies'), 'strategy')

    def test_escape(self):
        self.assertEqual(pyndri.escape('hello (world)'),
                         'hello world')
        self.assertEqual(pyndri.escape('hello.world'),
                         'hello world')
        self.assertEqual(pyndri.escape('hello:world'),
                         'hello world')

    def test_tokenize(self):
        self.assertEqual(pyndri.tokenize('hello world foo bar'),
                         ('hello', 'world', 'foo', 'bar'))

        self.assertEqual(pyndri.tokenize('hello-world'),
                         ('hello', 'world'))

        self.assertEqual(pyndri.tokenize('hello.world'),
                         ('hello',))
        self.assertEqual(pyndri.tokenize(pyndri.escape('hello.world')),
                         ('hello', 'world'))

        self.assertEqual(pyndri.tokenize('hello "world"'),
                         ('hello', 'world',))

        self.assertRaises(OSError,
                          lambda: pyndri.tokenize('hello (world)'))

        self.assertEqual(pyndri.tokenize(pyndri.escape('hello \'world\'')),
                         ('hello', 'world',))
        self.assertEqual(pyndri.tokenize(pyndri.escape('hello/world')),
                         ('hello', 'world',))


class IndriTest(PyndriTest):

    CORPUS = """<DOC>
<DOCNO>lorem</DOCNO>
<TEXT>
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis in magna id urna lobortis tristique sed eget sem. Fusce fringilla nibh in tortor venenatis, eget laoreet metus luctus. Maecenas velit arcu, ullamcorper quis mauris ut, posuere consectetur nibh. Integer sodales mi consectetur arcu gravida porta. Cras maximus sapien non nisi cursus, sit amet sollicitudin tortor porttitor. Nulla scelerisque eu est at fringilla. Cras felis elit, cursus in efficitur a, varius id nisl. Morbi lorem nulla, ornare vitae porta eget, convallis vestibulum nulla. Integer vestibulum et sem ac scelerisque.
</TEXT>
</DOC>
<DOC>
<DOCNO>hamlet</DOCNO>
<TEXT>
ACT I  SCENE I. Elsinore. A platform before the castle.  FRANCISCO at his post. Enter to him BERNARDO BERNARDO Who's there? FRANCISCO Nay, answer me: stand, and unfold yourself. BERNARDO Long live the king! FRANCISCO Bernardo? BERNARDO He. FRANCISCO You come most carefully upon your hour. BERNARDO 'Tis now struck twelve; get thee to bed, Francisco. FRANCISCO For this relief much thanks: 'tis bitter cold, And I am sick at heart.
</TEXT>
</DOC>
<DOC>
<DOCNO>romeo</DOCNO>
<TEXT>
ACT I  PROLOGUE  Two households, both alike in dignity, In fair Verona, where we lay our scene, From ancient grudge break to new mutiny, Where civil blood makes civil hands unclean. From forth the fatal loins of these two foes A pair of star-cross'd lovers take their life; Whose misadventured piteous overthrows Do with their death bury their parents' strife. The fearful passage of their death-mark'd love, And the continuance of their parents' rage, Which, but their children's end, nought could remove, Is now the two hours' traffic of our stage; The which if you with patient ears attend, What here shall miss, our toil shall strive to mend. SCENE I. Verona. A public place.  Enter SAMPSON and GREGORY, of the house of Capulet, armed with swords and bucklers SAMPSON Gregory, o' my word, we'll not carry coals. GREGORY No, for then we should be colliers. SAMPSON I mean, an we be in choler, we'll draw. GREGORY Ay, while you live, draw your neck out o' the collar. SAMPSON I strike quickly, being moved. GREGORY But thou art not quickly moved to strike. SAMPSON A dog of the house of Montague moves me. GREGORY To move is to stir; and to be valiant is to stand: therefore, if thou art moved, thou runn'st away. SAMPSON A dog of that house shall move me to stand: I will take the wall of any man or maid of Montague's. GREGORY That shows thee a weak slave; for the weakest goes to the wall. SAMPSON True; and therefore women, being the weaker vessels, are ever thrust to the wall: therefore I will push Montague's men from the wall, and thrust his maids to the wall. GREGORY The quarrel is between our masters and us their men. SAMPSON 'Tis all one, I will show myself a tyrant: when I have fought with the men, I will be cruel with the maids, and cut off their heads. GREGORY The heads of the maids? SAMPSON Ay, the heads of the maids, or their maidenheads; take it in what sense thou wilt. GREGORY They must take it in sense that feel it. SAMPSON Me they shall feel while I am able to stand: and 'tis known I am a pretty piece of flesh. GREGORY 'Tis well thou art not fish; if thou hadst, thou hadst been poor John. Draw thy tool! here comes two of the house of the Montagues. SAMPSON My naked weapon is out: quarrel, I will back thee. GREGORY How! turn thy back and run? SAMPSON Fear me not. GREGORY No, marry; I fear thee! SAMPSON Let us take the law of our sides; let them begin. GREGORY I will frown as I pass by, and let them take it as they list. SAMPSON Nay, as they dare. I will bite my thumb at them; which is a disgrace to them, if they bear it. Enter ABRAHAM and BALTHASAR  ABRAHAM Do you bite your thumb at us, sir? SAMPSON I do bite my thumb, sir. ABRAHAM Do you bite your thumb at us, sir? SAMPSON [Aside to GREGORY] Is the law of our side, if I say ay? GREGORY No. SAMPSON No, sir, I do not bite my thumb at you, sir, but I bite my thumb, sir. GREGORY Do you quarrel, sir? ABRAHAM Quarrel sir! no, sir. SAMPSON If you do, sir, I am for you: I serve as good a man as you.
</TEXT>
</DOC>
"""

    INDRI_CONFIG = """<parameters>
<index>index/</index>
<memory>1024M</memory>
<storeDocs>true</storeDocs>
<corpus><path>corpus.trectext</path><class>trectext</class></corpus>
<stemmer><name>krovetz</name></stemmer>
</parameters>"""

    def setUp(self):
        super(IndriTest, self).setUp()

        self.test_dir = tempfile.mkdtemp()

        with open(os.path.join(self.test_dir,
                               'corpus.trectext'),
                  'w', encoding='latin1') as f:
            f.write(self.CORPUS)

        with open(os.path.join(self.test_dir,
                               'IndriBuildIndex.conf'), 'w') as f:
            f.write(self.INDRI_CONFIG)

        with open(os.devnull, "w") as f:
            ret = subprocess.call(['IndriBuildIndex', 'IndriBuildIndex.conf'],
                                  stdout=f,
                                  cwd=self.test_dir)

        self.assertEqual(ret, 0)

        self.index_path = os.path.join(self.test_dir, 'index')
        self.assertTrue(os.path.exists(self.index_path))

        self.index = pyndri.Index(self.index_path)

    def test_1empty(self):
        pass  # Empty test to verify reference counting.

    def test_with(self):
        with pyndri.open(self.index_path) as index:
            self.assertTrue(isinstance(index, pyndri.Index))

    def test_repr(self):
        self.assertEqual(
            repr(self.index),
            '<pyndri.Index of 3 documents>')

    def test_meta(self):
        self.assertEqual(self.index.document_base(), 1)
        self.assertEqual(self.index.maximum_document(), 4)

        self.assertEqual(len(self.index), 3)

        self.assertEqual(self.index.path,
                         os.path.join(self.test_dir, 'index'))

    def test_process_term(self):
        self.assertEqual(self.index.process_term('HELLO'), 'hello')

        # Applies Krovetz stemming.
        self.assertEqual(self.index.process_term('Greek'), 'greece')

    def test_simple_query(self):
        self.assertEqual(
            self.index.query('ipsum'),
            ((1, -6.373564749941117),))

        self.assertEqual(
            self.index.query('his'),
            ((2, -5.794010932279138),
             (3, -5.972370287143733)))

    def test_query_documentset(self):
        self.assertEqual(
            self.index.query(
                'his',
                document_set=map(
                    operator.itemgetter(1),
                    self.index.document_ids(['hamlet']))),
            ((2, -5.794010932279138),))

    def test_query_results_requested(self):
        self.assertEqual(
            self.index.query(
                'his',
                results_requested=1),
            ((2, -5.794010932279138),))

    def test_query_snippets(self):
        self.assertEqual(
            self.index.query('ipsum', include_snippets=True),
            ((1, -6.373564749941117,
              'Lorem IPSUM dolor sit amet, consectetur '
              'adipiscing\nelit. Duis...'),))

    def test_document_length(self):
        self.assertEqual(self.index.document_length(1), 88)
        self.assertEqual(self.index.document_length(2), 71)
        self.assertEqual(self.index.document_length(3), 573)

    def test_raw_dictionary(self):
        token2id, id2token, id2df = self.index.get_dictionary()
        id2tf = self.index.get_term_frequencies()

        self.assertEqual(len(token2id), len(id2token))

        for token, idx in token2id.items():
            self.assertEqual(id2token[idx], token)

            self.assertGreaterEqual(id2df[idx], 1)
            self.assertGreaterEqual(id2tf[idx], 1)

    def test_expression_list(self):
        expression_list_ = self.index.expression_list("#od1(consectetur adipiscing)")
        self.assertEqual(expression_list_, {'lorem': 1})
        expression_list_ = self.index.expression_list("#od1(your)")
        self.assertEqual(expression_list_, {'hamlet': 1, 'romeo': 3})

    def test_document_terms(self):
        first_id, first_tokens = self.index.document_terms(1)

        self.assertEqual(first_id, 'lorem')

        self.assertEqual(len(first_tokens), 88)

        self.assertEqual(
            list(first_tokens),
            ['lorem', 'ipsum', 'dolor', 'sit', 'amet', 'consectetur',
             'adipisc', 'elit', 'dui', 'in', 'magna', 'id', 'urna',
             'loborti', 'tristique', 'sed', 'eget', 'sem', 'fusce',
             'fringilla', 'nibh', 'in', 'tort', 'venenati', 'eget',
             'laoreet', 'metu', 'luctu', 'maecena', 'velit', 'arcu',
             'ullamcorper', 'qui', 'mauri', 'ut', 'posuere', 'consectetur',
             'nibh', 'integer', 'sodale', 'mi', 'consectetur', 'arcu',
             'gravida', 'porta', 'cra', 'maximu', 'sapien', 'non', 'nisi',
             'cursu', 'sit', 'amet', 'sollicitudin', 'tort', 'porttitor',
             'nulla', 'scelerisque', 'eu', 'est', 'at', 'fringilla', 'cra',
             'feli', 'elit', 'cursu', 'in', 'efficitur', 'a', 'variu', 'id',
             'nisl', 'morbi', 'lorem', 'nulla', 'ornare', 'vitae', 'porta',
             'eget', 'convalli', 'vestibulum', 'nulla', 'integer',
             'vestibulum', 'et', 'sem', 'ac', 'scelerisque'])

    def test_iter_index(self):
        ext_doc_ids = [
            self.index.document_terms(int_doc_id)[0]
            for int_doc_id in range(
                self.index.document_base(),
                self.index.maximum_document())]

        self.assertEqual(ext_doc_ids,
                         ['lorem', 'hamlet', 'romeo'])

    def test_query_environment(self):
        env = pyndri.QueryEnvironment(
            self.index,
            rules=('method:linear,collectionLambda:0.4,documentLambda:0.2',))

        self.assertEqual(
            env.query('ipsum'),
            ((1, -4.911066480756002),))

        self.assertEqual(
            env.query('his'),
            ((2, -4.6518844642777),
             (3, -6.1469416959076195)))

        another_env = pyndri.QueryEnvironment(
            self.index,
            rules=('method:linear,collectionLambda:1.0,documentLambda:0.0',))

        self.assertEqual(
            another_env.query('ipsum'),
            ((1, -6.595780513961311),))

        self.assertEqual(
            another_env.query('his'),
            ((3, -5.902633333401366),
             (2, -5.902633333401366)))

    def test_tfidf(self):
        env = pyndri.TFIDFQueryEnvironment(self.index)

        self.assertEqual(
            env.query('ipsum'),
            ((1, 0.7098885466183784),))

        self.assertEqual(
            env.query('his'),
            ((2, 0.16955104430709383),
             (3, 0.07757942488345955)))

    def test_okapi(self):
        env = pyndri.OkapiQueryEnvironment(self.index)

        self.assertEqual(
            env.query('ipsum'),
            ((1, 0.691753771033259),))

        self.assertEqual(
            env.query('his'),
            ((3, -0.3292246306130194),
             (2, -0.7195255702901702)))

    def test_tokenize(self):
        self.assertEqual(
            self.index.tokenize('hello world foo bar'),
            ['hello', 'world', 'foo', 'bar'])

        # Tokenization also applied stemming.
        self.assertEqual(
            self.index.tokenize('strategies predictions'),
            ['strategy', 'prediction'])

    def tearDown(self):
        shutil.rmtree(self.test_dir)
        del self.index
        self.index = None

        super(IndriTest, self).tearDown()

if __name__ == '__main__':
    unittest.main()
