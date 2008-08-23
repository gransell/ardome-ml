"""AML External Access

This module can be installed on any system regardless of whether it has 
access to the binary AML runtime or not. The classes and functionality 
provided are primarily related to client communications, utilities and 
cross platform GUI clients (Tkinter based)."""

from amlx.client import client, parse
import amlx.transport as transport
import amlx.audio as audio
import amlx.playlist as playlist
