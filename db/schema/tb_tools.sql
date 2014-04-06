drop table if exists tools;

create table tools 
(
  tool_id           int not null auto_increment,
  tool_address      varchar(255),
  tool_name         varchar(20),
  tool_status       varchar(20),  -- IN_USE, FREE or DISABLED
  tool_restrictions varchar(20),  -- UNRESTRICTED or RESTRICTED
  tool_status_text  varchar(255), -- If tool_status=DISABLED, holds the reason why (free text)
  tool_pph          int unsigned, -- Cost - pence per hour
  primary key (tool_id),
  unique (tool_name)
);
