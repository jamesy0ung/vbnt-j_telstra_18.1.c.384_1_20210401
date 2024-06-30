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
package org.apache.felix.dm.test;

import java.io.PrintStream;

import junit.framework.Assert;

/**
 * Helper class to make sure that steps in a test happen in the correct order. Instantiate
 * this class and subsequently invoke <code>step(nr)</code> with steps starting at 1. You
 * can also have threads wait until you arrive at a certain step.
 */
public class Ensure {
    private static final boolean DEBUG = true;
    private static long INSTANCE = 0;
    private static final int RESOLUTION = 100;
    private static PrintStream STREAM = System.out;
    int step = 0;
    
    public Ensure() {
        if (DEBUG) {
            INSTANCE++;
        }
    }

    public void setStream(PrintStream output) {
        STREAM = output;
    }
    
    /**
     * Mark this point as step <code>nr</code>.
     * 
     * @param nr the step we are in
     */
    public synchronized void step(int nr) {
        step++;
        Assert.assertEquals(nr, step);
        if (DEBUG) {
            STREAM.println("[Ensure " + INSTANCE + "] step " + step + " [" + currentThread() + "]");
        }
        notifyAll();
    }
    
    /**
     * Mark this point as the next step.
     */
    public synchronized void step() {
        step++;
        if (DEBUG) {
            STREAM.println("[Ensure " + INSTANCE + "] next step " + step + " [" + currentThread() + "]");
        }
        notifyAll();
    }

    /**
     * Wait until we arrive at least at step <code>nr</code> in the process, or fail if that
     * takes more than <code>timeout</code> milliseconds. If you invoke wait on a thread,
     * you are effectively assuming some other thread will invoke the <code>step(nr)</code>
     * method.
     * 
     * @param nr the step to wait for
     * @param timeout the number of milliseconds to wait
     */
    public synchronized void waitForStep(int nr, int timeout) {
        final int initialTimeout = timeout;
        if (DEBUG) {
            STREAM.println("[Ensure " + INSTANCE + "] waiting for step " + nr + " [" + currentThread() + "]");
        }
        while (step < nr && timeout > 0) {
            try {
                wait(RESOLUTION);
                timeout -= RESOLUTION;
            }
            catch (InterruptedException e) {}
        }
        if (step < nr) {
            throw new IllegalStateException("Timed out waiting for " + initialTimeout + " ms for step " + nr + ", we are still at step " + step);
        }
        if (DEBUG) {
            STREAM.println("[Ensure " + INSTANCE + "] arrived at step " + nr + " [" + currentThread() + "]");
        }
    }
    
    private String currentThread() {
        Thread thread = Thread.currentThread();
        return thread.getId() + " " + thread.getName();
    }
    
    public static Runnable createRunnableStep(final Ensure ensure, final int nr) {
        return new Runnable() { public void run() { ensure.step(nr); }};
    }
}
