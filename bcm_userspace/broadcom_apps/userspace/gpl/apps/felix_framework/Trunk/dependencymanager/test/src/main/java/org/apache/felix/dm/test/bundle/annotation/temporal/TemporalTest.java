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
package org.apache.felix.dm.test.bundle.annotation.temporal;

import org.apache.felix.dm.annotation.api.Service;
import org.apache.felix.dm.annotation.api.Start;
import org.apache.felix.dm.annotation.api.Stop;
import org.apache.felix.dm.annotation.api.ServiceDependency;
import org.apache.felix.dm.test.bundle.annotation.sequencer.Sequencer;

/**
 * Service using an annotated Temporal Service dependendency.
 */
@Service(provide = {})
public class TemporalTest implements Runnable
{
    Thread m_thread;

    @ServiceDependency
    Sequencer m_sequencer;

    @ServiceDependency(timeout = 1000L, filter = "(test=temporal)")
    Runnable m_service;

    @Start
    protected void start()
    {
        m_thread = new Thread(this);
        m_thread.start();
    }

    @Stop
    protected void stop()
    {
        m_thread.interrupt();
        try
        {
            m_thread.join();
        }
        catch (InterruptedException e)
        {
        }
    }

    public void run()
    {
        m_service.run();
        m_sequencer.waitForStep(2, 15000);
        m_service.run(); // we should block here
        m_sequencer.waitForStep(4, 15000);
        try
        {
            m_service.run(); // should raise IllegalStateException
        }
        catch (IllegalStateException e)
        {
            m_sequencer.step(5);
        }
    }
}
