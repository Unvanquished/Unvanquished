/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2015, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/
#ifndef COMMON_CRYPTO_H_
#define COMMON_CRYPTO_H_


namespace Crypto {

	using Data = std::vector<uint8_t>;

	Data RandomData( std::size_t bytes );

	inline Data String(const std::string& string)
	{
		return Data(string.begin(), string.end());
	}

	inline std::string String(const Data string)
	{
		return std::string(string.begin(), string.end());
	}

	namespace Encoding {

		/*
		 * Translates binary data into a hexadecimal string
		 */
		Data HexEncode( const Data& input );
		/*
		 * Translates a hexadecimal string into binary data
		 * PRE: input is a valid hexadecimal string
		 * POST: output contains the decoded string
		 * Returns true on success
		 * Note: If the decoding fails, output is unchanged
		 */
		bool HexDecode( const Data& input, Data& output );

		/*
		 * Translates binary data into a base64 encoded string
		 */
		Data Base64Encode( const Data& input );

		/*
		 * Translates a base64 encoded string into binary data
		 * PRE: input is a valid Base64 string
		 * POST: output contains the decoded string
		 * Returns true on success
		 * Note: If the decoding fails, output is unchanged
		 */
		bool Base64Decode( const Data& input, Data& output );


	} // namespace Encoding


	namespace Hash {
		Data Sha256( const Data& input );
		Data Md5( const Data& input );
	} // namespace Hash

	/*
	 * Adds PKCS#7 padding to the data
	 */
	void AddPadding( Data& target, std::size_t block_size = 8 );

	/*
	 * Encrypts using the AES256 algorithm
	 * PRE: Key is 256-bit (32 octects) long
	 * POST: output contains the encrypted data
	 * Notes: plain_text will be padded using the PKCS#7 algorthm,
	 *        if the decoding fails, output is unchanged
	 * Returns true on success
	 */
	bool Aes256Encrypt( Data plain_text, const Data& key, Data& output );

	/*
	 * Encrypts using the AES256 algorithm
	 * PRE: Key is 256-bit (32 octects) long,
	 *      cypher_text.size() is an integer multiple of the AES block size
	 * POST: output contains the decrypted data
	 * Note: If the decoding fails, output is unchanged
	 * Returns true on success
	 */
	bool Aes256Decrypt( Data cypher_text, const Data& key, Data& output );

} // namespace Crypto

#endif // COMMON_CRYPTO_H_
