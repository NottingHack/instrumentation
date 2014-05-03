drop table if exists tl_tool_usages;

create table tl_tool_usages 
(
  usage_id            int not null auto_increment,
  member_id           int,
  tool_id             int,
  usage_start         datetime,     -- Sign on time
  usage_duration      int,          -- Use duration in seconds
  usage_active_time   int,          -- Amount of time tool active for, where applicable (e.g. laser tube time)
  usage_status        varchar(20),  -- IN_PROGRESS, COMPLETE or CHARGED
  primary key (usage_id)
);
