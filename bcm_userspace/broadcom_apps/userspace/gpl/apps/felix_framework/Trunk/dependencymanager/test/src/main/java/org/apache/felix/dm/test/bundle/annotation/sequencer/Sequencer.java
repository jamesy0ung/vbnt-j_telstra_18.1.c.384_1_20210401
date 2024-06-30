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
package org.apache.felix.dm.test.bundle.annotation.sequencer;

/**
 * Service used to ensure that steps happen in the expected order.
 */
public interface Sequencer
{
    /**
     * Proceed with the next step.
     */
    void step();
    
    /**
     * Crosses a given step.
     * @param step the step we are crossing
     */
    void step(int step);
    
    /**
     * Wait for a given step to occur.
     * @param nr the step we are waiting for
     * @param timeout max milliseconds to wait.
     */
    void waitForStep(int step, int timeout);
}
