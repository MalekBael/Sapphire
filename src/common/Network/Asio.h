#pragma once

// Use standalone Asio (no Boost dependency).
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

// Silence deprecated Asio APIs across the project (optional, but matches modern usage).
#ifndef ASIO_NO_DEPRECATED
#define ASIO_NO_DEPRECATED
#endif

#include <asio.hpp>
