/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file LICENSE.rst or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include <cstddef>
#include <string>
#include <vector>

#include <cm/optional>

/** \class cmProcessOutput
 * \brief Decode text data to internal encoding.
 *
 * cmProcessOutput is used to decode text output from external process
 * using external encoding to our internal encoding.
 */
class cmProcessOutput
{
public:
  enum Encoding
  {
    None,
    Auto,
    UTF8,
    ANSI,
    OEM
  };

  /**
   * Find encoding enum value for given encoding \a name.
   * \param name a encoding name.
   * \return encoding enum value or Auto if \a name was not found.
   */
  static cm::optional<Encoding> FindEncoding(std::string const& name);

  /// The code page that is used as internal encoding to which we will encode.
  static unsigned int defaultCodepage;

  /**
   * A class constructor.
   * \param encoding external process encoding from which we will decode.
   * \param maxSize a maximal size for process output buffer. It should match
   * to KWSYSPE_PIPE_BUFFER_SIZE. If text we decode is same size as \a maxSize
   * then we will check for incomplete character at end of buffer and
   * we will not return last incomplete character. This character will be
   * returned with next DecodeText() call. To disable this behavior specify
   * 0 as \a maxSize.
   */
  cmProcessOutput(Encoding encoding = Auto, unsigned int maxSize = 1024);
  ~cmProcessOutput() = default;
  /**
   * Decode \a raw string using external encoding to internal
   * encoding in \a decoded.
   * \a id specifies which internal buffer to use. This is important when we
   * are decoding both stdout and stderr from process output and we need to
   * keep incomplete characters in separate buffers for each stream.
   * \return true if successfully decoded \a raw to \a decoded or false if not.
   */
  bool DecodeText(std::string raw, std::string& decoded, size_t id = 0);
  /**
   * Decode \a data with \a length from external encoding to internal
   * encoding in \a decoded.
   * \param data a pointer to process output text data.
   * \param length a size of data buffer.
   * \param decoded a string which will contain decoded text.
   * \param id an internal buffer id to use.
   * \return true if successfully decoded \a data to \a decoded or false if
   * not.
   */
  bool DecodeText(char const* data, size_t length, std::string& decoded,
                  size_t id = 0);
  /**
   * \overload
   */
  bool DecodeText(std::vector<char> raw, std::vector<char>& decoded,
                  size_t id = 0);

private:
#if defined(_WIN32)
  unsigned int codepage;
  unsigned int bufferSize;
  std::vector<std::string> rawparts;
  bool DoDecodeText(std::string raw, std::string& decoded, wchar_t* lastChar);
#endif
};
