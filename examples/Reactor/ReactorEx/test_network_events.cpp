// $Id$
//
// ============================================================================
//
// = LIBRARY
//    examples
// 
// = FILENAME
//    test_network_events.cpp
//
// = DESCRIPTION
//
//    This application tests Reactor to make sure that it responds
//    correctly to different kinds of network events.
//
//    The test starts off by creating a Network_Listener, that listens
//    for connections at ACE_DEFAULT_SERVER_PORT. When a client
//    connects, a Network_Handler is created. Network_Handler reads
//    messages off the socket and prints them out. This is done until
//    the remote side shuts down. Multiple clients can connect at the
//    same time.
//
//    Events tested in this example includes ACCEPT, READ, and CLOSE masks. 
//
//    To run this example, start an instance of this example and
//    connect to it using telnet (to port
//    ACE_DEFAULT_SERVER_PORT(10002)). 
//
// = AUTHOR
//    Irfan Pyarali
// 
// ============================================================================

#include "ace/Reactor.h"
#include "ace/WFMO_Reactor.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"

// Globals for this test
int stop_test = 0;
ACE_Reactor reactor;

class Network_Handler : public ACE_Event_Handler
{
public:
  Network_Handler (ACE_SOCK_Stream &s);
  // Default constructor

  virtual int handle_input (ACE_HANDLE handle);
  virtual int handle_close (ACE_HANDLE handle,
			    ACE_Reactor_Mask close_mask);
  virtual ACE_HANDLE  get_handle (void) const;

  ACE_SOCK_Stream stream_;

};

Network_Handler::Network_Handler (ACE_SOCK_Stream &s)
  : stream_ (s)
{
  this->reactor (&::reactor);

  ACE_Reactor_Mask mask = ACE_Event_Handler::READ_MASK | ACE_Event_Handler::CLOSE_MASK;
  ACE_ASSERT (this->reactor ()->register_handler (this, 
						  mask) == 0);
}

ACE_HANDLE  
Network_Handler::get_handle (void) const
{
  return this->stream_.get_handle ();
}

int
Network_Handler::handle_input (ACE_HANDLE handle)
{
  ACE_DEBUG ((LM_DEBUG, "Network_Handler::handle_input handle = %d\n", handle));

  char message[BUFSIZ];
  int result = this->stream_.recv (message, sizeof message);
  if (result > 0)
    {
      message[result] = 0;
      ACE_DEBUG ((LM_DEBUG, "Remote message: %s\n", message));
      return 0;
    }
  else
    {
      ACE_DEBUG ((LM_DEBUG, "Problems in receiving data, result = %d", result));
      return -1;
    }
}

int
Network_Handler::handle_close (ACE_HANDLE handle,
			       ACE_Reactor_Mask close_mask)
{
  ACE_DEBUG ((LM_DEBUG, "Network_Handler::handle_close handle = %d\n", handle));

  // Called because of remote shutdown
  if (close_mask == ACE_Event_Handler::CLOSE_MASK)
    {
      ACE_Reactor_Mask mask = ACE_Event_Handler::DONT_CALL | ACE_Event_Handler::ALL_EVENTS_MASK;
      this->reactor ()->remove_handler (this, mask);
    }
  
  this->stream_.close ();
  
  delete this;

  return 0;
}

class Network_Listener : public ACE_Event_Handler
{
public:
  Network_Listener (void);
  // Default constructor

  virtual int handle_input (ACE_HANDLE handle);
  virtual int handle_close (ACE_HANDLE handle,
			    ACE_Reactor_Mask close_mask);
  ACE_HANDLE get_handle (void) const;

  ACE_INET_Addr local_address_;
  ACE_SOCK_Acceptor acceptor_;
};

Network_Listener::Network_Listener (void)
  : local_address_ (ACE_DEFAULT_SERVER_PORT),
    acceptor_ (local_address_, 1)
{
  this->reactor (&::reactor);
  ACE_ASSERT (this->reactor ()->register_handler (this, 
						  ACE_Event_Handler::ACCEPT_MASK) == 0);
}

ACE_HANDLE  
Network_Listener::get_handle (void) const
{
  return this->acceptor_.get_handle ();
}

int
Network_Listener::handle_input (ACE_HANDLE handle)
{
  ACE_DEBUG ((LM_DEBUG, "Network_Listener::handle_input handle = %d\n", handle));

  ACE_INET_Addr remote_address;
  ACE_SOCK_Stream stream;

  int reset_new_handle = 0;
#if defined (ACE_WIN32)
  // Try to find out if the implementation of the reactor that we are
  // using is the WFMO_Reactor. If so we need to reset the event
  // association for the newly created handle. This is because the
  // newly created handle will inherit the properties of the listen
  // handle, including its event associations.
  if (dynamic_cast <ACE_WFMO_Reactor *> (this->reactor ()->implementation ()))
    reset_new_handle = 1;
#endif /* ACE_WIN32 */
  
  ACE_ASSERT (this->acceptor_.accept (stream, // stream
                                      &remote_address, // remote address
                                      0, // timeout
                                      1, // restart
                                      reset_new_handle  // reset new handler
                                      ) == 0);
  
  ACE_DEBUG ((LM_DEBUG, "Remote connection from: "));
  remote_address.dump ();

  Network_Handler *handler = new Network_Handler (stream);
  
  return 0;
}

int
Network_Listener::handle_close (ACE_HANDLE handle,
				ACE_Reactor_Mask close_mask)
{
  ACE_DEBUG ((LM_DEBUG, "Network_Listener::handle_close handle = %d\n", handle));

  this->acceptor_.close ();
  return 0;
}

int 
main (int, char *[])
{
  Network_Listener listener;

  int result = 0;
  while (!stop_test && result != -1)
    {
      result = reactor.handle_events ();
    }
  return 0;
};
