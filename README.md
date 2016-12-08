u2fh
====

This is light wrapper around Yubico libu2fh host library for host side interaction with yubikeys.

API
==

The API very closely resembles the C library API, with the awkward bits (for PHP) removed:

	namespace u2fh;

	/*
	* Shall discover plugged in devices, returning the index of the last device
	* Note: may return 0
	* Note: shall throw an exception on error
	*/
    int discover(void);

	/*
	* Shall ping the selected $dev to determine if it is alive
	* Note: shall throw an exception on error
	*/
	bool ping(int dev);

	/*
	* 
	*/
	string register(string challenge, string origin, bool presence = false);

	/*
	* 
	*/
	string authenticate(string challenge, string origin, bool presence = false);

	/*
	* Shall send a command via serial bus
	* Note: shall throw an exception on error
	*/
	string sendrecv(int dev, int cmd, string data);

	/*
	* Shall return a description of the selected $dev
	* Note: shall throw an exception on error
	*/
	string describe(int dev);

	/*
	* Constant commands
	*/
	const PING;
	const MSG;
	const LOCK;
	const INIT;
	const WINK;
	const ERROR;
	const VFIRST;
	const VLAST;


