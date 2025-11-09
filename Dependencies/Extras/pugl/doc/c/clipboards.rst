.. default-domain:: c
.. highlight:: c

################
Using Clipboards
################

Clipboards provide a way to transfer data between different views,
including views in different processes.
A clipboard transfer is a multi-step event-driven process,
where the sender and receiver can negotiate a mutually supported data format.

*******
Copying
*******

Data can be copied to the general clipboard with :func:`puglSetClipboard`.
The `MIME type <https://www.iana.org/assignments/media-types/media-types.xhtml>`_ of the data must be specified.
Commonly supported types are ``text/plain`` for plain text,
and `text/uri-list <http://amundsen.com/hypermedia/urilist/>`_ for lists of URIs (including local files).

For example, a string can be copied to the clipboard by setting the general clipboard to ``text/plain`` data:

.. code-block:: c

   const char* someString = "Copied string";

   puglSetClipboard(view,
                    "text/plain",
                    someString,
                    strlen(someString));

*******
Pasting
*******

Data from a clipboard can be pasted to a view using :func:`puglPaste`:

.. code-block:: c

   puglPaste(view);

This initiates a data transfer from the clipboard to the view if possible.
If data is available,
the view will be sent a :enumerator:`PUGL_DATA_OFFER` event to begin the transfer.

**************
Receiving Data
**************

A data transfer from a clipboard to a view begins with the view receiving a :enumerator:`PUGL_DATA_OFFER` event.
This indicates that data (possibly in several formats) is being offered to a view,
which can either "accept" or "reject" it:

.. code-block:: c

   case PUGL_DATA_OFFER:
     onDataOffer(view, &event->offer);
     break;

When handling this event,
:func:`puglGetNumClipboardTypes` and :func:`puglGetClipboardType` can be used to enumerate the available data types:

.. code-block:: c

   static void
   onDataOffer(PuglView* view, const PuglEventDataOffer* event)
   {
     size_t numTypes = puglGetNumClipboardTypes(view, clipboard);

     for (uint32_t t = 0; t < numTypes; ++t) {
       const char* type = puglGetClipboardType(view, t);
       printf("Offered type: %s\n", type);
     }
   }

If the view supports one of the data types,
it can accept the offer with :func:`puglAcceptOffer`:

.. code-block:: c

   for (uint32_t t = 0; t < numTypes; ++t) {
     const char* type = puglGetClipboardType(view, t);
     if (!strcmp(type, "text/uri-list")) {
       puglAcceptOffer(view, event, t);
     }
   }

When an offer is accepted,
the data will be transferred and converted if necessary,
then the view will be sent a :enumerator:`PUGL_DATA` event.
When the data event is received,
the data can be fetched with :func:`puglGetClipboard`:

.. code-block:: c

   case PUGL_DATA:
     onData(view, &event->data);
     break;

   // ...

   static void
   onData(PuglView* view, const PuglEventData* event)
   {
     uint32_t typeIndex = event->typeIndex;

     const char* type = puglGetClipboardType(view, typeIndex);

     fprintf(stderr, "Received data type: %s\n", type);

     if (!strcmp(type, "text/plain")) {
       size_t      len  = 0;
       const void* data = puglGetClipboard(view, typeIndex, &len);

       printf("Dropped: %s\n", (const char*)data);
     }
   }
