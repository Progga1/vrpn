#ifndef	TRACKER_H
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

// NOTE: a vrpn tracker must call user callbacks with tracker data (pos and
//       ori info) which represent the transformation xfSourceFromSensor.
//       This means that the pos info is the position of the origin of
//       the sensor coord sys in the source coord sys space, and the
//       quat represents the orientation of the sensor relative to the
//       source space (ie, its value rotates the source's axes so that
//       they coincide with the sensor's)

// to use time synched tracking, just pass in a sync connection to the 
// client and the server

#include "vrpn_Connection.h"

// flush discards characters in buffer
// drain blocks until they are written
extern int vrpn_open_commport(char *portname, long baud);
extern int vrpn_flush_input_buffer( int comm );
extern int vrpn_flush_output_buffer( int comm );
extern int vrpn_drain_output_buffer( int comm );

// tracker status flags
#define TRACKER_SYNCING		(2)
#define TRACKER_REPORT_READY 	(1)
#define TRACKER_PARTIAL 	(0)
#define TRACKER_RESETTING	(-1)
#define TRACKER_FAIL 	 	(-2)

#define TRACKER_MAX_SENSORS	(20)
// index for the change_list that should be called for all sensors
#define ALL_SENSORS		TRACKER_MAX_SENSORS
// this is the maximum for indices into sensor-specific change lists
// it is 1 more than the number of sensors because the last list is
// used to specify all sensors
#define TRACKER_MAX_SENSOR_LIST (TRACKER_MAX_SENSORS + 1)

class vrpn_Tracker {
  public:
   vrpn_Tracker(char *name, vrpn_Connection *c = NULL);
   virtual void mainloop(void) = 0;	// Handle getting any reports
   virtual ~vrpn_Tracker() {};

   int read_config_file(FILE *config_file, char *tracker_name);
   void print_latest_report(void);
   // a tracker server should call the following to register the
   // default xform and workspace request handlers
   int register_server_handlers(void);
   void get_local_t2r(double *vec, double *quat);
   void get_local_u2s(int sensor, double *vec, double *quat);
   static int handle_t2r_request(void *userdata, vrpn_HANDLERPARAM p);
   static int handle_u2s_request(void *userdata, vrpn_HANDLERPARAM p);
   static int handle_workspace_request(void *userdata, vrpn_HANDLERPARAM p);
   static int handle_update_rate_request (void *, vrpn_HANDLERPARAM);

   vrpn_Connection *connectionPtr();

  protected:
   vrpn_Connection *connection;         // Used to send messages
   long my_id;				// ID of this tracker to connection
   long position_m_id;			// ID of tracker position message
   long velocity_m_id;			// ID of tracker velocity message
   long accel_m_id;			// ID of tracker acceleration message
   long tracker2room_m_id;		// ID of tracker tracker2room message
   long unit2sensor_m_id;		// ID of tracker unit2sensor message
   long request_t2r_m_id;		// ID of tracker2room request message
   long request_u2s_m_id;		// ID of unit2sensor request message
   long request_workspace_m_id;		// ID of workspace request message
   long workspace_m_id;			// ID of workspace message
   long update_rate_id;			// ID of update rate message
					

   // Description of the next report to go out
   int sensor;			// Current sensor
   double pos[3], quat[4];	// Current position, (x,y,z), (qx,qy,qz,qw)
   double vel[3], vel_quat[4];	// Current velocity and dQuat/vel_quat_dt
   double vel_quat_dt;          // delta time (in secs) for vel_quat
   double acc[3], acc_quat[4];	// Current acceleration and d2Quat/acc_quat_dt2
   double acc_quat_dt;          // delta time (in secs) for acc_quat
   struct timeval timestamp;	// Current timestamp

   double tracker2room[3], tracker2room_quat[4]; // Current t2r xform
   long num_sensors;
   double unit2sensor[TRACKER_MAX_SENSORS][3];
   double unit2sensor_quat[TRACKER_MAX_SENSORS][4]; // Current u2s xforms

   // bounding box for the tracker workspace (in tracker space)
   // these are the points with (x,y,z) minimum and maximum
   // note: we assume the bounding box edges are aligned with the tracker
   // coordinate system
   double workspace_min[3], workspace_max[3];

   int status;		// What are we doing?

   virtual int encode_to(char *buf);	 // Encodes the position report
   // Not all trackers will call the velocity and acceleration packers
   virtual int encode_vel_to(char *buf); // Encodes the velocity report
   virtual int encode_acc_to(char *buf); // Encodes the acceleration report
   virtual int encode_tracker2room_to(char *buf); // Encodes the tracker2room
   virtual int encode_unit2sensor_to(char *buf); // and unit2sensor xforms
   virtual int encode_workspace_to(char *buf); // Encodes workspace info
};

#ifndef VRPN_CLIENT_ONLY
#ifndef _WIN32
#define VRPN_TRACKER_BUF_SIZE 100

class vrpn_Tracker_Serial : public vrpn_Tracker {
  public:
   vrpn_Tracker_Serial(char *name, vrpn_Connection *c,
		char *port = "/dev/ttyS1", long baud = 38400);
  protected:
   char portname[VRPN_TRACKER_BUF_SIZE];
   long baudrate;
   int serial_fd;

   unsigned char buffer[VRPN_TRACKER_BUF_SIZE];// Characters read in from the tracker so far
   unsigned bufcount;		// How many characters in the buffer?

   int read_available_characters(unsigned char *buffer, int count);
   virtual void get_report(void) = 0;
   virtual void reset(void) = 0;
};
#endif  // _WIN32
#endif  // VRPN_CLIENT_ONLY


class vrpn_Tracker_NULL: public vrpn_Tracker {
  public:
   vrpn_Tracker_NULL(char *name, vrpn_Connection *c, int sensors = 1,
	float Hz = 1.0);
   virtual void mainloop(void);
  protected:
   float	update_rate;
   int		num_sensors;
};




//----------------------------------------------------------
// ************** Users deal with the following *************

// User routine to handle a tracker position update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	double		pos[3];		// Position of the sensor
	double		quat[4];	// Orientation of the sensor
} vrpn_TRACKERCB;
typedef void (*vrpn_TRACKERCHANGEHANDLER)(void *userdata,
					 const vrpn_TRACKERCB info);

// User routine to handle a tracker velocity update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	double		vel[3];		// Velocity of the sensor
	double		vel_quat[4];	// Future Orientation of the sensor
        double          vel_quat_dt;    // delta time (in secs) for vel_quat
} vrpn_TRACKERVELCB;
typedef void (*vrpn_TRACKERVELCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERVELCB info);

// User routine to handle a tracker acceleration update.  This is called when
// the tracker callback is called (when a message from its counterpart
// across the connetion arrives).

typedef	struct {
	struct timeval	msg_time;	// Time of the report
	long		sensor;		// Which sensor is reporting
	double		acc[3];		// Acceleration of the sensor
	double		acc_quat[4];	// ?????
        double          acc_quat_dt;    // delta time (in secs) for acc_quat
        
} vrpn_TRACKERACCCB;
typedef void (*vrpn_TRACKERACCCHANGEHANDLER)(void *userdata,
					     const vrpn_TRACKERACCCB info);

// User routine to handle a tracker room2tracker xform update. This is called
// when the tracker callback is called (when a message from its counterpart
// across the connection arrives).

typedef struct {
	struct timeval	msg_time;	// Time of the report
	double	tracker2room[3];	// position offset
	double	tracker2room_quat[4];	// orientation offset
} vrpn_TRACKERTRACKER2ROOMCB;
typedef void (*vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER)(void *userdata,
					const vrpn_TRACKERTRACKER2ROOMCB info);

typedef struct {
        struct timeval  msg_time;       // Time of the report
	long	sensor;			// Which sensor this is the xform for
        double  unit2sensor[3];        // position offset
        double  unit2sensor_quat[4];   // orientation offset
} vrpn_TRACKERUNIT2SENSORCB;
typedef void (*vrpn_TRACKERUNIT2SENSORCHANGEHANDLER)(void *userdata,
                                        const vrpn_TRACKERUNIT2SENSORCB info);

typedef struct {
	struct timeval  msg_time;       // Time of the report
	double workspace_min[3];	// minimum corner of box (tracker CS)
	double workspace_max[3];	// maximum corner of box (tracker CS)
} vrpn_TRACKERWORKSPACECB;
typedef void (*vrpn_TRACKERWORKSPACECHANGEHANDLER)(void *userdata,
					const vrpn_TRACKERWORKSPACECB info);
#ifndef VRPN_CLIENT_ONLY


// This is a virtual device that plays back pre-recorded sensor data.

class vrpn_Tracker_Canned: public vrpn_Tracker {
  
  public:
  
   vrpn_Tracker_Canned (char * name, vrpn_Connection * c,
                        char * datafile);

   virtual ~vrpn_Tracker_Canned (void);
   virtual void mainloop (void);

  protected:

   virtual void get_report (void) { /* do nothing */ }
   virtual void reset (void);

  private:

   void copy (void);

   FILE * fp;
   int totalRecord;
   vrpn_TRACKERCB t;
   int current;

};
#endif  // VRPN_CLIENT_ONLY




// Open a tracker that is on the other end of a connection
// and handle updates from it.  This is the type of tracker that user code will
// deal with.

class vrpn_Tracker_Remote: public vrpn_Tracker {
  public:
	// The name of the tracker to connect to
	vrpn_Tracker_Remote(char *name);

        // unregister all of the handlers registered with the connection
        ~vrpn_Tracker_Remote();

	// request room from tracker xforms
	int request_t2r_xform(void);
	// request all available sensor from unit xforms
	int request_u2s_xform(void);
	// request workspace bounding box
	int request_workspace(void);

	// set rate of p/v/a updates from the tracker
	int set_update_rate (double samplesPerSecond);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// **** to register handlers for all sensors: ****

	// (un)Register a callback handler to handle a position change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERCHANGEHANDLER handler);

	// (un)Register a callback handler to handle a velocity change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERVELCHANGEHANDLER handler);

	// (un)Register a callback handler to handle an acceleration change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERACCCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERACCCHANGEHANDLER handler);

	// (un)Register a callback handler to handle a tracker2room change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler);

	// (un)Register a callback handler to handle a unit2sensor change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler);

	// **** to register handlers for specific sensors: ****

        // (un)Register a callback handler to handle a position change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERCHANGEHANDLER handler, int sensor);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERCHANGEHANDLER handler, int sensor);

        // (un)Register a callback handler to handle a velocity change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERVELCHANGEHANDLER handler, int sensor);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERVELCHANGEHANDLER handler, int sensor);

        // (un)Register a callback handler to handle an acceleration change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERACCCHANGEHANDLER handler, int sensor);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERACCCHANGEHANDLER handler, int sensor);

        // (un)Register a callback handler to handle a unit2sensor change
        virtual int register_change_handler(void *userdata,
                vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, int sensor);
        virtual int unregister_change_handler(void *userdata,
                vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler, int sensor);

	// **** to get workspace information ****
	// (un)Register a callback handler to handle a workspace change
	virtual int register_change_handler(void *userdata,
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RTCS {
		void				*userdata;
		vrpn_TRACKERCHANGEHANDLER	handler;
		struct vrpn_RTCS		*next;
	} vrpn_TRACKERCHANGELIST;
	vrpn_TRACKERCHANGELIST	*change_list[TRACKER_MAX_SENSORS + 1];

	typedef	struct vrpn_RTVCS {
		void				*userdata;
		vrpn_TRACKERVELCHANGEHANDLER	handler;
		struct vrpn_RTVCS		*next;
	} vrpn_TRACKERVELCHANGELIST;
	vrpn_TRACKERVELCHANGELIST *velchange_list[TRACKER_MAX_SENSORS + 1];

	typedef	struct vrpn_RTACS {
		void				*userdata;
		vrpn_TRACKERACCCHANGEHANDLER	handler;
		struct vrpn_RTACS		*next;
	} vrpn_TRACKERACCCHANGELIST;
	vrpn_TRACKERACCCHANGELIST *accchange_list[TRACKER_MAX_SENSORS + 1];

	typedef struct vrpn_RTT2RCS {
		void				*userdata;
		vrpn_TRACKERTRACKER2ROOMCHANGEHANDLER handler;
		struct vrpn_RTT2RCS		*next;
	} vrpn_TRACKERTRACKER2ROOMCHANGELIST;
	vrpn_TRACKERTRACKER2ROOMCHANGELIST *tracker2roomchange_list;

	typedef struct vrpn_RTU2SCS {
		void                            *userdata;
		vrpn_TRACKERUNIT2SENSORCHANGEHANDLER handler;
		struct vrpn_RTU2SCS		*next;
    } vrpn_TRACKERUNIT2SENSORCHANGELIST;
	vrpn_TRACKERUNIT2SENSORCHANGELIST *unit2sensorchange_list[TRACKER_MAX_SENSORS + 1];
	
	typedef struct vrpn_RTWSCS {
		void				*userdata;
		vrpn_TRACKERWORKSPACECHANGEHANDLER handler;
		struct vrpn_RTWSCS	*next;
	} vrpn_TRACKERWORKSPACECHANGELIST;
	vrpn_TRACKERWORKSPACECHANGELIST *workspacechange_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_vel_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_acc_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_tracker2room_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
	static int handle_unit2sensor_change_message(void *userdata,
                        vrpn_HANDLERPARAM p);
	static int handle_workspace_change_message(void *userdata,
			vrpn_HANDLERPARAM p);
};


#define TRACKER_H
#endif
