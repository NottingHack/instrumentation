drop table if exists tl_members_tools;

create table tl_members_tools 
(
  member_tool_id    int not null auto_increment,
  member_id         int,
  tool_id           int,
  member_id_induct  int,         -- member_id of inductor
  mt_date_inducted  datetime,
  mt_access_level   varchar(20), -- USER, INDUCTOR or MAINTAINER
  primary key (member_tool_id),
  unique (tool_id, member_id)
) ENGINE = InnoDB; 