<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select 
    year(tu.usage_start) as Year, 
    monthname(tu.usage_start) as Month, 
    sec_to_time(sum(tu.usage_duration)) as "Time (hh:mm:ss)"
  from tl_tool_usages tu
  where tu.tool_id = 1
    and tu.usage_duration > 0
  group by year(tu.usage_start),  monthname(tu.usage_start)
  order by year(tu.usage_start) desc,  month(tu.usage_start) desc
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>