/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package org.apache.felix.karaf.admin.command;

import org.apache.felix.gogo.commands.Option;
import org.apache.felix.gogo.commands.Argument;
import org.apache.felix.gogo.commands.Command;

@Command(scope = "admin", name = "connect", description = "Connects to an existing container instance.")
public class ConnectCommand extends AdminCommandSupport {

    @Argument(index = 0, name="name", description="The name of the container instance", required = true, multiValued = false)
    private String instance = null;

    @Option(name="-u", aliases={"--username"}, description="Remote user name (Default: karaf)", required = false, multiValued = false)
    private String username = "karaf";

    @Option(name="-p", aliases={"--password"}, description="Remote user password (Default: karaf)", required = false, multiValued = false)
    private String password = "karaf";

    protected Object doExecute() throws Exception {
        int port = getExistingInstance(instance).getPort();
        session.execute("ssh -l " + username + " -P " + password + " -p " + port + " localhost");
        return null;
    }
}
