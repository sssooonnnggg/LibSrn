#include "SQLiteHelper.h"

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

#include "utility.h"
#include "DebugTools.h"

#pragma comment(lib, "libeay32.lib")
#pragma warning(disable:4996)

#define PAGESIZE 1024
#define PBKDF2_ITER 4000
#define FILE_HEADER_SZ 16

bool SQLiteHelper::DecryptDatabaseFile(
	const std::wstring& dbFile, 
	const std::wstring& outputFile,
	const char* password)
{
	int i, csz, tmp_csz, key_sz, iv_sz;
	FILE *infh, *outfh;
	int read, written;
	unsigned char *inbuffer, *outbuffer, *salt, *out, *key, *iv;
	EVP_CIPHER *evp_cipher;
	EVP_CIPHER_CTX ectx;

	OpenSSL_add_all_algorithms();

	evp_cipher = (EVP_CIPHER *) EVP_get_cipherbyname("aes-256-cbc");

	key_sz = EVP_CIPHER_key_length(evp_cipher);
	key = (unsigned char*)malloc(key_sz);

	iv_sz = EVP_CIPHER_iv_length(evp_cipher);
	iv = (unsigned char*)malloc(iv_sz);

	inbuffer = (unsigned char*) malloc(PAGESIZE);
	outbuffer = (unsigned char*) malloc(PAGESIZE);
	salt = (unsigned char*)malloc(FILE_HEADER_SZ);

	infh = _wfopen(dbFile.c_str(), L"rb");
	outfh = _wfopen(outputFile.c_str(), L"wb");

	if ( !infh )
	{
		DebugTools::OutputDebugPrintfW(L"[SQLiteHelper] Open File Failed. [%s]\r\n", dbFile.c_str());
		return false;
	}

	if ( !infh )
	{
		DebugTools::OutputDebugPrintfW(L"[SQLiteHelper] Open File Failed. [%s]\r\n", outputFile.c_str());
		return false;
	}

	read = fread(inbuffer, 1, PAGESIZE, infh);  /* read the first page */
	memcpy(salt, inbuffer, FILE_HEADER_SZ); /* first 16 bytes are the random database salt */

	PKCS5_PBKDF2_HMAC_SHA1(password, strlen(password), salt, FILE_HEADER_SZ, PBKDF2_ITER, key_sz, key);

	memset(outbuffer, 0, PAGESIZE);
	out = outbuffer;

	memcpy(iv, inbuffer + PAGESIZE - iv_sz, iv_sz); /* last iv_sz bytes are the initialization vector */

	EVP_CipherInit(&ectx, evp_cipher, NULL, NULL, 0);
	EVP_CIPHER_CTX_set_padding(&ectx, 0);
	EVP_CipherInit(&ectx, NULL, key, iv, 0);
	EVP_CipherUpdate(&ectx, out, &tmp_csz, inbuffer + FILE_HEADER_SZ, PAGESIZE - iv_sz - FILE_HEADER_SZ);
	csz = tmp_csz;  
	out += tmp_csz;
	EVP_CipherFinal(&ectx, out, &tmp_csz);
	csz += tmp_csz;
	EVP_CIPHER_CTX_cleanup(&ectx);

	fwrite("SQLite format 3\0", 1, FILE_HEADER_SZ, outfh);
	fwrite(outbuffer, 1, PAGESIZE - FILE_HEADER_SZ, outfh);

	for(i = 1; (read = fread(inbuffer, 1, PAGESIZE, infh)) > 0 ;i++) {
		memcpy(iv, inbuffer + PAGESIZE - iv_sz, iv_sz); /* last iv_sz bytes are the initialization vector */
		memset(outbuffer, 0, PAGESIZE);
		out = outbuffer;

		EVP_CipherInit(&ectx, evp_cipher, NULL, NULL, 0);
		EVP_CIPHER_CTX_set_padding(&ectx, 0);
		EVP_CipherInit(&ectx, NULL, key, iv, 0);
		EVP_CipherUpdate(&ectx, out, &tmp_csz, inbuffer, PAGESIZE - iv_sz);
		csz = tmp_csz;  
		out += tmp_csz;
		EVP_CipherFinal(&ectx, out, &tmp_csz);
		csz += tmp_csz;
		EVP_CIPHER_CTX_cleanup(&ectx);

		fwrite(outbuffer, 1, PAGESIZE, outfh);
	}

	fclose(infh);
	fclose(outfh);

	free(inbuffer);
	free(outbuffer);
	free(key);
	free(salt);
	free(iv);

	return true;
}

SQLiteHelper::SQLiteHelper()
	: m_sqlite(NULL)
{

}

bool SQLiteHelper::ConnectToDatabase(const std::wstring& fileName)
{
	int errNo = 0;

	if ( SQLITE_OK != (errNo = sqlite3_open16((void*)fileName.c_str(), &m_sqlite)) )
	{
		DebugTools::OutputDebugPrintfW(
			L"[SQLiteHelper] Open database file failed. [%s] [%d]\r\n", 
			fileName.c_str(), 
			errNo);
		return false;
	}

	return true;
}

void SQLiteHelper::CloseDatabase()
{
	if ( m_sqlite )
	{
		sqlite3_close(m_sqlite);
		m_sqlite = NULL;
	}
}

bool SQLiteHelper::Exec(const std::wstring& sql)
{
	int errNo = sqlite3_prepare16(
		m_sqlite,
		(const void*)sql.c_str(), 
		2*sql.length(),
		&m_state,
		NULL);

	if ( SQLITE_OK != errNo )
	{
		DebugTools::OutputDebugPrintfW(
			L"[SQLiteHelper] Execute sql failed. [%s] [%d]\r\n", 
			sql.c_str(), 
			errNo);
		return false;
	}

	return true;
}

bool SQLiteHelper::Step()
{
	int ret = sqlite3_step(m_state);
	
	if ( SQLITE_DONE == ret )
		return false;

	if ( SQLITE_ROW != ret )
		DebugTools::OutputDebugPrintfW(
		L"[SQLiteHelper] Step sql failed. [%d]\r\n", 
		ret);

	return true;
}

const char* SQLiteHelper::GetData(int column, __int64* size)
{
	*size = sqlite3_column_bytes(m_state, column);
	const void* data = sqlite3_column_blob(m_state, column);
	return (const char*)sqlite3_column_blob(m_state, column);
}

std::wstring SQLiteHelper::GetText(int column)
{
	const void* data = sqlite3_column_text16(m_state, column);

	if ( NULL == data )
		return L"";

	return std::wstring(static_cast<const wchar_t*>(data));
}

__int64 SQLiteHelper::GetValue(int column)
{
	return sqlite3_column_int64(m_state, column);
}

bool SQLiteHelper::Finalize()
{
	int errNo = sqlite3_finalize(m_state);

	if ( SQLITE_OK != errNo )
	{
		DebugTools::OutputDebugPrintfW(
			L"[SQLiteHelper] Finalize state failed. [%d]\r\n", 
			errNo);
		return false;
	}

	return true;
}