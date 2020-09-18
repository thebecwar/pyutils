#########
# Mutex #
#########
from pyutils import WinApiMutex

# constructor params: name, take_ownership (all optional, keyword args allowed)
# take_ownership: specifies whether to obtain the mutex on creation
mutex = WinApiMutex("Name", True)

# acquire the mutex (optional parameter: timeout in milliseconds)
mutex.acquire(1000)

# release the mutex
mutex.release()

#########
# Event #
#########
from pyutils import WinApiEvent

#constructor params: name, auto_reset, initial_state (all optional, keyword args allowed)
# auto_reset: 
#	True - when signalled, release 1 thread and go back to non-signalled.
#	False - (default value) when signalled, release threads until reset
# initial_state:
#	True - event starts in signalled state
#	False - event starts in unsignalled state
event = WinApiEvent("Name", True, False)

#signal the event
event.set()
#reset the event
event.reset()
#wait for the event to be signalled (optional parameter: timeout in milliseconds)
event.wait(1000)

#############
# Semaphore #
#############
from pyutils import WinApiSemaphore

#constructor params: name, initial_count, maximum_count (All required, keyword args allowed)
# initial_count: starting count for semaphore. Equivalent to CreateSemaphore's lInitialCount parameter.
# maximum_count: maximum count for semaphore. Equivalent to CreateSemaphore's lMaximumCount parameter.
semaphore = WinApiSemaphore("Name", 2, 2)

#acquire the semaphore (optional parameter: timeout in milliseconds)
semaphore.acquire(100)
semaphore.acquire(100)
semaphore.acquire(100) # TimeoutError

#release the semaphore (optional parameter: release count for multiple acquisitions)
semaphore.release(2)
semaphore.release() # WinError 298 Too many posts were made to a semaphore

######################
# Memory Mapped File #
######################
from pyutils import MemoryMappedFile

#Constructor params: name, size
# size: size in bytes for the memory. The file and mapping will be created with the specified size
memory = MemoryMappedFile("Name", 1000)

#Assignment: only values 0-255 supported, single byte assignment only
memory[0] = 100

#Fetching: single indexing returns an int()
tmp = memory[0]
# tmp == 100

#Fetching: slices return a bytes() object for the specified range. (Not buffered. When sliced, copies the memory to a new python object)
tmp = memory[0:2]
# tmp = b'\x64\x00'

