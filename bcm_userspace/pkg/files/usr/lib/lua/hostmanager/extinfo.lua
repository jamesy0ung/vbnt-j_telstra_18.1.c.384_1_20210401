#!/usr/bin/env lua

-- ************* COPYRIGHT AND CONFIDENTIALITY INFORMATION **********************
-- **                                                                          **
-- ** Copyright (c) 2016 Technicolor                                           **
-- ** All Rights Reserved                                                      **
-- **
-- ** This program is free software; you can redistribute it and/or modify **
-- ** it under the terms of the GNU General Public License version 2 as    **
-- ** published by the Free Software Foundation.                           **                                                                          **
-- **                                                                          **
-- ******************************************************************************

local popen = io.popen
local match = string.match

local tonumber = tonumber

local M = {}

function M.get_extinfo()
	local result = {}
	local appcount = {}
	local f = popen ("dpictl dev")

	if f == nil then
		return result
	end

	for line in f:lines() do
		local info = {  }
		local mac, devname, upPkt, upByte, dnPkt, dnByte =
			match(line, '([^,]+),([^,]+),(%d+),(%d+),(%d+),(%d+)')

		if (devname ~= "N/A") then
			info.os = devname
		end
		info.stats = {  up = {packets = tonumber(upPkt),
				      bytes = tonumber(upByte)},
			        down = {
				      packets = tonumber(dnPkt),
			              bytes = tonumber(dnByte)}}
		info.apps = { }
		result[mac] = info
		appcount[mac] = 0
	end

	f:close()

	f = popen ("dpictl appinst")

	if f == nil then
		return result
	end

	for line in f:lines() do
		local appid,appname,mac,upPkt,upByte,dnPkt,dnByte =
			match(line, '^(%d+),([^,]+),([^,]+),[^,]+,(%d+),(%d+),(%d+),(%d+)')
		if appid then
			appcount[mac] = appcount[mac] + 1
			result[mac].apps["app" .. appcount[mac]]= { name = appname,
							    stats = {  up = {packets = tonumber(upPkt),
									     bytes = tonumber(upByte)},
								       down = {packets = tonumber(dnPkt),
									       bytes = tonumber(dnByte)}}}
		end
	end

	f:close()

	return result
end

return M
