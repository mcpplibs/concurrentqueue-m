// Included first in the global module fragment of src/concurrentqueue.cppm
// (a fragment holds only inclusions).
//
// The queue's member templates call upstream's `static` helpers
// (details::likely, details::circular_less_than …), so exporting them exposes
// TU-local entities of the global module fragment. GCC accepts this as an
// extension, with a warning per template, and importers instantiate the
// templates (every test here does); Clang accepts it as is.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wexpose-global-module-tu-local"
#endif
