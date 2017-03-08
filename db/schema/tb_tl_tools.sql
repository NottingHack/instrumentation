drop table if exists tl_tools;

create table tl_tools 
(
  tool_id             int not null auto_increment,
  tool_address        varchar(255),
  tool_name           varchar(20),
  tool_status         varchar(20),  -- IN_USE, FREE or DISABLED
  tool_restrictions   varchar(20),  -- UNRESTRICTED or RESTRICTED
  tool_status_text    varchar(255), -- If tool_status=DISABLED, holds the reason why (free text)
  tool_pph            int unsigned, -- Cost - pence per hour
  tool_booking_length int unsigned, -- default booking length for this tool
  tool_length_max     int unsigned, -- maximum amount of time a booking can be made for
  tool_bookings_max   int unsigned, -- maximum number of bookings a user can have at any one time
  tool_calendar       varchar(255), -- id of google calendar
  tool_cal_poll_ival  int unsigned not null default 0, -- Calendar polling interval: How often to poll/publish booking information in seconds. 0=never
  primary key (tool_id),
  unique (tool_name)
) ENGINE = InnoDB; 
