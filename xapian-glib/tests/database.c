#include <glib.h>
#include "xapian-glib.h"

typedef struct {
  char *db_path;
} DatabaseFixture;

static void
database_fixture_setup (DatabaseFixture *fixture,
                        gconstpointer    user_data)
{
  g_mkdir_with_parents (g_get_home_dir (), 0700);
  fixture->db_path = g_build_filename (g_get_home_dir (), "test-db", NULL);
}

static void
database_fixture_teardown (DatabaseFixture *fixture,
                           gconstpointer    user_data)
{
  g_free (fixture->db_path);
}

static void
database_new_empty (void)
{
  XapianDatabase *db = xapian_database_new (NULL);
  char * uuid;

  g_assert_nonnull (db);

  g_object_add_weak_pointer (G_OBJECT (db), (gpointer *) &db);

  g_assert (XAPIAN_IS_DATABASE (db));

  uuid = xapian_database_get_uuid (db);
  g_assert_nonnull (uuid);
  g_assert_cmpstr (uuid, ==, "");
  g_free (uuid);

  g_object_unref (db);
  g_assert_null (db);
}

static void
database_new_nonexistent (void)
{
  GError *error = NULL;
  XapianDatabase *db = xapian_database_new_with_path ("doesnotexist", &error);

  g_assert_null (db);
  g_assert_error (error, XAPIAN_ERROR, XAPIAN_ERROR_DATABASE_NOT_FOUND);

  g_error_free (error);
}

static void
database_writable_new (DatabaseFixture *fixture,
                       gconstpointer    user_data)
{
  GError *error = NULL;
  XapianWritableDatabase *db =
    xapian_writable_database_new (fixture->db_path,
                                  XAPIAN_DATABASE_ACTION_CREATE,
                                  &error);

  g_assert_nonnull (db);

  g_object_add_weak_pointer (G_OBJECT (db), (gpointer *) &db);

  g_assert_true (XAPIAN_IS_WRITABLE_DATABASE (db));
  g_assert_true (XAPIAN_IS_DATABASE (db));

  g_object_unref (db);
  g_assert_null (db);

  g_assert_true (g_file_test (fixture->db_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR));
}

static void
database_writable_backend_glass (DatabaseFixture *fixture,
                                 gconstpointer    user_data)
{
  GError *error = NULL;
  XapianWritableDatabase *db =
    xapian_writable_database_new_with_backend (fixture->db_path,
                                               XAPIAN_DATABASE_ACTION_CREATE,
                                               XAPIAN_DATABASE_BACKEND_GLASS,
                                               &error);

  g_assert_nonnull (db);

  g_object_add_weak_pointer (G_OBJECT (db), (gpointer *) &db);

  g_assert_true (XAPIAN_IS_WRITABLE_DATABASE (db));
  g_assert_true (XAPIAN_IS_DATABASE (db));

  g_object_unref (db);
  g_assert_null (db);

  char *termlist_path = g_build_filename (fixture->db_path, "termlist.glass", NULL);

  g_assert_true (g_file_test (fixture->db_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR));
  g_assert_true (g_file_test (termlist_path, G_FILE_TEST_EXISTS));

  g_free (termlist_path);
}

static void
database_writable_flags_no_termlist (DatabaseFixture *fixture,
                                     gconstpointer    user_data)
{
  GError *error = NULL;
  XapianWritableDatabase *db =
    xapian_writable_database_new_full (fixture->db_path,
                                       XAPIAN_DATABASE_ACTION_CREATE,
                                       XAPIAN_DATABASE_BACKEND_GLASS,
                                       XAPIAN_DATABASE_FLAGS_NO_TERMLIST,
                                       &error);

  g_assert_nonnull (db);

  g_object_add_weak_pointer (G_OBJECT (db), (gpointer *) &db);

  g_assert_true (XAPIAN_IS_WRITABLE_DATABASE (db));
  g_assert_true (XAPIAN_IS_DATABASE (db));

  g_object_unref (db);
  g_assert_null (db);

  char *termlist_path = g_build_filename (fixture->db_path, "termlist.glass", NULL);

  g_assert_true (g_file_test (fixture->db_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR));
  g_assert_false (g_file_test (termlist_path, G_FILE_TEST_EXISTS));

  g_free (termlist_path);
}

static void
database_writable_all_terms (DatabaseFixture *fixture,
                             gconstpointer    user_data)
{
  GError *error = NULL;
  char *term;
  XapianWritableDatabase *wdb =
    xapian_writable_database_new_full (fixture->db_path,
                                       XAPIAN_DATABASE_ACTION_CREATE,
                                       XAPIAN_DATABASE_BACKEND_GLASS,
                                       0,
                                       &error);

  g_assert_nonnull (wdb);

  g_object_add_weak_pointer (G_OBJECT (wdb), (gpointer *) &wdb);

  XapianDocument *doc = xapian_document_new ();

  g_object_add_weak_pointer (G_OBJECT (doc), (gpointer *) &doc);

  xapian_document_add_term (doc, "one");
  xapian_document_add_term (doc, "two");

  xapian_writable_database_add_document (wdb, doc, NULL, &error);
  g_object_unref (wdb);
  g_assert_null (wdb);

  g_object_unref (doc);
  g_assert_null (doc);

  XapianDatabase *db =
    xapian_database_new_with_path (fixture->db_path, &error);

  g_object_add_weak_pointer (G_OBJECT (db), (gpointer *) &db);

  // Test with NULL for the prefix.
  XapianTermIterator *it = xapian_database_enumerate_all_terms (db, NULL);
  g_assert_true (xapian_term_iterator_next (it));
  term = xapian_term_iterator_get_term_name (it);
  g_assert_cmpstr (term, ==, "one");
  g_free (term);
  g_assert_true (xapian_term_iterator_next (it));
  term = xapian_term_iterator_get_term_name (it);
  g_assert_cmpstr (term, ==, "two");
  g_free (term);
  g_assert_false (xapian_term_iterator_next (it));
  g_object_unref (it);

  // Test with explicitly empty prefix.
  it = xapian_database_enumerate_all_terms (db, "");
  g_assert_true (xapian_term_iterator_next (it));
  term = xapian_term_iterator_get_term_name (it);
  g_assert_cmpstr (term, ==, "one");
  g_free (term);
  g_assert_true (xapian_term_iterator_next (it));
  term = xapian_term_iterator_get_term_name (it);
  g_assert_cmpstr (term, ==, "two");
  g_free (term);
  g_assert_false (xapian_term_iterator_next (it));
  g_object_unref (it);

  // Test with prefix.
  it = xapian_database_enumerate_all_terms (db, "t");
  g_assert_true (xapian_term_iterator_next (it));
  term = xapian_term_iterator_get_term_name (it);
  g_assert_cmpstr (term, ==, "two");
  g_free (term);
  g_assert_false (xapian_term_iterator_next (it));
  g_object_unref (it);

  // Test with prefix which doesn't match anything.
  it = xapian_database_enumerate_all_terms (db, "x");
  g_assert_false (xapian_term_iterator_next (it));
  g_object_unref (it);

  g_object_unref (db);
  g_assert_null (db);
}

static void
database_writable_freqs (DatabaseFixture *fixture,
                         gconstpointer    user_data)
{
  GError *error = NULL;
  XapianWritableDatabase *wdb =
    xapian_writable_database_new_full (fixture->db_path,
                                       XAPIAN_DATABASE_ACTION_CREATE,
                                       XAPIAN_DATABASE_BACKEND_GLASS,
                                       0,
                                       &error);

  g_assert_nonnull (wdb);

  g_object_add_weak_pointer (G_OBJECT (wdb), (gpointer *) &wdb);

  XapianDocument *doc = xapian_document_new ();

  g_object_add_weak_pointer (G_OBJECT (doc), (gpointer *) &doc);

  xapian_document_add_term (doc, "two");

  xapian_writable_database_add_document (wdb, doc, NULL, &error);

  xapian_document_add_term (doc, "one");

  xapian_writable_database_add_document (wdb, doc, NULL, &error);

  g_object_unref (wdb);
  g_assert_null (wdb);

  g_object_unref (doc);
  g_assert_null (doc);

  XapianDatabase *db =
    xapian_database_new_with_path (fixture->db_path, &error);

  g_object_add_weak_pointer (G_OBJECT (db), (gpointer *) &db);

  // Regression test: pre-condition checks for term not being NULL were inverted.
  g_assert_cmpint (xapian_database_get_term_freq (db, "one"), ==, 1);
  g_assert_cmpint (xapian_database_get_term_freq (db, "two"), ==, 2);
  g_assert_cmpint (xapian_database_get_collection_freq (db, "two"), ==, 2);
  g_assert_cmpint (xapian_database_get_collection_freq (db, "one"), ==, 1);

  g_object_unref (db);
  g_assert_null (db);
}

static void
database_writable_closed (DatabaseFixture *fixture,
                          gconstpointer    user_data)
{
  GError *error = NULL;
  XapianWritableDatabase *wdb =
    xapian_writable_database_new_full (fixture->db_path,
                                       XAPIAN_DATABASE_ACTION_CREATE,
                                       XAPIAN_DATABASE_BACKEND_GLASS,
                                       0,
                                       &error);

  g_assert_nonnull (wdb);

  g_object_add_weak_pointer (G_OBJECT (wdb), (gpointer *) &wdb);

  xapian_database_close (XAPIAN_DATABASE (wdb));

  XapianDocument *doc = xapian_document_new ();

  g_object_add_weak_pointer (G_OBJECT (doc), (gpointer *) &doc);

  xapian_document_add_term (doc, "term");

  g_assert_false (xapian_writable_database_add_document (wdb, doc, NULL, &error));

  g_assert_error (error, XAPIAN_ERROR, XAPIAN_ERROR_DATABASE_CLOSED);

  g_object_unref (doc);
  g_assert_null (doc);

  g_object_unref (wdb);
  g_assert_null (wdb);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/database/new/empty", database_new_empty);
  g_test_add_func ("/database/new/non-existent", database_new_nonexistent);

  g_test_add ("/database/writable/new",
              DatabaseFixture, NULL,
              database_fixture_setup,
              database_writable_new,
              database_fixture_teardown);

  g_test_add ("/database/writable/backend/glass",
              DatabaseFixture, NULL,
              database_fixture_setup,
              database_writable_backend_glass,
              database_fixture_teardown);

  g_test_add ("/database/writable/flags/no-termlist",
              DatabaseFixture, NULL,
              database_fixture_setup,
              database_writable_flags_no_termlist,
              database_fixture_teardown);

  g_test_add ("/database/writable/all_terms",
              DatabaseFixture, NULL,
              database_fixture_setup,
              database_writable_all_terms,
              database_fixture_teardown);

  g_test_add ("/database/writable/freqs",
              DatabaseFixture, NULL,
              database_fixture_setup,
              database_writable_freqs,
              database_fixture_teardown);

  g_test_add ("/database/writable/closed",
              DatabaseFixture, NULL,
              database_fixture_setup,
              database_writable_closed,
              database_fixture_teardown);

  return g_test_run ();
}
